// If you are not compiling with the gcc option --std=gnu99, then
// uncomment the following line or you might get a compiler warning
//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "shellcommand.h"
#include "bg_process.h"
#include "prompt.h"
#include "parsecommand.h"
#include "expand.h"

int g_fgmode = 0;

void handle_SIGTSTP(int signo){
    char* message1 = "Entering foreground-only mode\n";
    char* message2 = "Exiting foreground-only mode\n";

    g_fgmode = (g_fgmode == 0 ? 1: 0);
    if(g_fgmode){
        write(STDOUT_FILENO, message1, 31);
    }
    else {
        write(STDOUT_FILENO, message2, 30);
    }
}

int main(int argc, char *argv[])
{
    // char * startLine = ":"
    char* readBuffer = calloc(sizeof(char), 2048);
    char* expandedString = calloc(sizeof(char), 2048);
    struct ShellCommand* command;
    int f_running = 1;
    int last_exit_status = 0;
    int f_terminated = 0;
    size_t bufferSize = 2048;

    int i_fd;
    int o_fd;
    int i_d_result;
    int o_d_result;

    struct BgProcess* bg_list = NULL;
    struct BgProcess* cur_bg_proc;
    struct BgProcess* prev_bg_proc;
    struct BgProcess* new_bg_process = NULL;

    struct sigaction SIGINT_action = {0};
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;

    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;

    // Starting cwd
    char current_working_directory [FILENAME_MAX];
    getcwd(current_working_directory, FILENAME_MAX);

    while(f_running){

        if (bg_list != NULL){

            // Search through the linked list of background processes running
            cur_bg_proc = bg_list;
            prev_bg_proc = NULL;

            while (cur_bg_proc != NULL){
                cur_bg_proc->bg_done = waitpid(cur_bg_proc->bg_pid, &cur_bg_proc->bg_status, WNOHANG);

                // background process has finished
                if(cur_bg_proc->bg_done != 0){
                    // Update user
                    if(WIFEXITED(cur_bg_proc->bg_status)){
                        f_terminated = 0;
                        printf("background pid %d is done: exit value %d\n", (int)cur_bg_proc->bg_pid, WEXITSTATUS(cur_bg_proc->bg_status));
                        fflush(stdout);
                    }
                    else {
                        f_terminated = 1;
                        printf("background pid %d is done: terminated by signal %d\n", (int)cur_bg_proc->bg_pid, WTERMSIG(cur_bg_proc->bg_status));
                        fflush(stdout);
                    }

                    // remove from list
                    // This is the head
                    if(prev_bg_proc == NULL){
                        bg_list = cur_bg_proc->next;
                    }
                    //Not the head
                    else{
                        prev_bg_proc->next = cur_bg_proc->next;
                    }

                    // free memory
                    free(cur_bg_proc);
                    cur_bg_proc = NULL;
                }
                else{
                    // Increment forward
                    prev_bg_proc = cur_bg_proc;
                    cur_bg_proc = cur_bg_proc->next;

                }
            }
        }

        // Clear these string
        memset(readBuffer, '\0', 2048);
        memset(expandedString, '\0', 2048);

        // Get user input from shell input
        getInputFromPrompt(readBuffer, &bufferSize);

        // Expand the string if necessary
        expandVariable(readBuffer, expandedString);

        // Parse the string into the ShellCommand struct
        command = parseCommand(expandedString, g_fgmode);

        if(command->command == NULL){
            continue;
        }

        // Run Commands
        // First, built in smallsh commands
        // cd command
        if (strcmp(command->command, "cd") == 0){
            if (command->args[1] == NULL){
                chdir(getenv("HOME"));
                getcwd(current_working_directory, FILENAME_MAX);
            }
            else{
                int chworks = chdir(command->args[1]);
                if (chworks == 0){
                    getcwd(current_working_directory, FILENAME_MAX);
                }
                else {
                    //No error handling apparently
                }
            }
        }

        // status command
        else if (strcmp(command->command, "status") == 0){
            if(f_terminated){
                printf("terminated by signal %d\n", last_exit_status);
                fflush(stdout);
            }
            else{
                printf("exit value %d\n", last_exit_status);
                fflush(stdout);
            }
        }

        // exit command
        else if (strcmp(command->command, "exit") == 0){
            // Kill all processes and jobs
            while (bg_list != NULL){
                cur_bg_proc = bg_list;
                kill(cur_bg_proc->bg_pid, 15);
                cur_bg_proc->bg_done = waitpid(cur_bg_proc->bg_pid, &cur_bg_proc->bg_status, 0);

                // remove from list
                bg_list = cur_bg_proc->next;

                // free memory
                free(cur_bg_proc);
                cur_bg_proc = NULL;
            }
            // Stop small shell
            f_running = 0;
        }

        // Use fork + execvp to execute all other commands
        else {
            // Do other commands
            pid_t child_pid = fork();

            switch(child_pid){
                case -1:
                    // Fork failed
                    perror("fork()");
                    break;
                case 0:
                    // Child Process
                    // Trigger correct signal handlers for SIGTSTP and SIGINT
                    SIGTSTP_action.sa_handler = SIG_IGN;
                    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

                    if(command->f_background_process){
                        SIGINT_action.sa_handler = SIG_IGN;
                        sigaction(SIGINT, &SIGINT_action, NULL);
                    }
                    else {
                        SIGINT_action.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &SIGINT_action, NULL);
                    }

                    // Redirect input from a file
                    if(command->f_input_file){
                        i_fd = open(command->input_file, O_RDONLY);
                        if(i_fd == -1){
                            perror("input open()");
                            exit(1);
                        }

                        i_d_result = dup2(i_fd, 0);
                        if(i_d_result == -1){
                            perror("input dup2()");
                            exit(1);
                        }
                    }
                    else if(command->f_background_process){
                        i_fd = open("/dev/null", O_RDONLY);
                        if(i_fd == -1){
                            perror("input open()");
                            exit(1);
                        }

                        i_d_result = dup2(i_fd, 0);
                        if(i_d_result == -1){
                            perror("input dup2()");
                            exit(1);
                        }
                    }

                    // Redirect output to a file
                    if (command->f_output_file){
                        o_fd = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                        if(o_fd == -1){
                            perror("output open()");
                            exit(1);
                        }

                        o_d_result = dup2(o_fd, 1);
                        if (o_d_result == -1){
                            perror("output dup2");
                            exit(1);
                        }
                    }
                    else if(command->f_background_process){
                        o_fd = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0640);
                        if(o_fd == -1){
                            perror("output open()");
                            exit(1);
                        }

                        o_d_result = dup2(o_fd, 1);
                        if (o_d_result == -1){
                            perror("output dup2");
                            exit(1);
                        }
                    }

                    // Run command
                    command->args[0] = calloc(strlen(command->command) + 1, sizeof(char));
                    strcpy(command->args[0], command->command);
                    execvp(command->command, command->args);
                    perror("execvp");
                    exit(1);
                    break;
                default:
                    //Parent process
                    // Trigger correct signal handlers
                    SIGINT_action.sa_handler = SIG_IGN;
                    sigaction(SIGINT, &SIGINT_action, NULL);
                    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

                    // Wait for foreground processes
                    if (!command->f_background_process){
                        child_pid = waitpid(child_pid, &last_exit_status, 0);

                        if(WIFEXITED(last_exit_status)){
                            last_exit_status = WEXITSTATUS(last_exit_status);
                            f_terminated = 0;
                        }
                        else if(WIFSIGNALED(last_exit_status)){
                            last_exit_status = WTERMSIG(last_exit_status);
                            f_terminated = 1;
                            printf("Child %d terminated by signal %d\n", (int)child_pid, last_exit_status);
                        fflush(stdout);
                        }
                    }

                    // Add a node to the linked list list of background processes
                    // Continue operation
                    else{
                        new_bg_process = malloc(sizeof(struct BgProcess));
                        new_bg_process->bg_pid = child_pid;

                        if (bg_list != NULL){
                            new_bg_process->next = bg_list;
                            bg_list = new_bg_process;
                        }
                        else{
                            new_bg_process->next = NULL;
                            bg_list = new_bg_process;
                        }

                        printf("background pid is %d\n", (int)child_pid);
                        fflush(stdout);
                    }
                    break;
            }
        }

        // TODO: Why do frees crash the program?
        // Free memory
        // memset(command->command, '\0', sizeof(command->command));
        // free(command->command);

        // for(int i=0; i<514; i++){
        //     if(command->args[i] != NULL){
        //         memset(command->args[i], '\0', sizeof(command->args[i]));
        //         free(command->args[i]);
        //     }
        // }

        // if(command->f_input_file){
        //     memset(command->input_file, '\0', sizeof(command->input_file));
        //     free(command->input_file);
        // }
        // if(command->f_output_file){
        //     memset(command->output_file, '\0', sizeof(command->output_file));
        //     free(command->output_file);
        // }

        // memset(command, '\0', sizeof(struct ShellCommand));
        // free(command);

    }

    // Free local memory
    free(readBuffer);
    free(expandedString);

    return EXIT_SUCCESS;
}