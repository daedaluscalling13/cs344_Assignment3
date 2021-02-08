// If you are not compiling with the gcc option --std=gnu99, then
// uncomment the following line or you might get a compiler warning
//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "shellcommand.h"
#include "bg_process.h"
#include "parsecommand.h"
#include "expand.h"

int main(int argc, char *argv[])
{
    // char * startLine = ":"
    char* readBuffer = calloc(sizeof(char), 2048);
    char* expandedString = calloc(sizeof(char), 2048);
    struct ShellCommand* command;
    int f_running = 1;
    int last_exit_status = 0;
    size_t bufferSize = 2048;

    int i_fd;
    int o_fd;
    int i_d_result;
    int o_d_result;

    struct BgProcess* bg_list = NULL;
    struct BgProcess* cur_bg_proc;
    struct BgProcess* prev_bg_proc;
    struct BgProcess* new_bg_process = NULL;

    // Starting cwd
    char current_working_directory [FILENAME_MAX];
    getcwd(current_working_directory, FILENAME_MAX);

    while(f_running){

        if (bg_list != NULL){

            // Search through the linked list of background processes running
            cur_bg_proc = bg_list;
            prev_bg_proc = NULL;
            printf("Current bg process list:\n");

            while (cur_bg_proc != NULL){
                printf("%d\n", (int)cur_bg_proc->bg_pid);

                cur_bg_proc->bg_done = waitpid(cur_bg_proc->bg_pid, &cur_bg_proc->bg_status, WNOHANG);

                // background process has finished
                if(cur_bg_proc->bg_done != 0){
                    // Update user
                    if(WIFEXITED(cur_bg_proc->bg_status)){
                        printf("background pid %d is done: exit value %d\n", (int)cur_bg_proc->bg_pid, WEXITSTATUS(cur_bg_proc->bg_status));
                        fflush(stdout);
                    }
                    else {
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

        // Clear this string
        memset(readBuffer, '\0', 2048);
        memset(expandedString, '\0', 2048);

        // Put semicolon on new line
        printf(": ");
        fflush(stdout);

        // Read in user input
        getline(&readBuffer, &bufferSize, stdin);
        fflush(stdin);

        // Remove newline char
        // Taken from the stackoverflow forum: https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
        char* newlinePos;
        if ((newlinePos = strchr(readBuffer, '\n')) != NULL){
            *newlinePos = '\0';
        }

        // Expand the string if necessary
        expandVariable(readBuffer, expandedString);

        // Parse the string into the ShellCommand struct
        command = parseCommand(expandedString);

        if(command->command == NULL){
            continue;
        }

        // DO COMMANDS
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
            printf("exit value %d\n", last_exit_status);
            fflush(stdout);
        }

        // exit command
        else if (strcmp(command->command, "exit") == 0){
            //TODO: Kill all processes and jobs
            f_running = 0;
        }

        // all other commands
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
                    command->args[0] = calloc(strlen(command->command) + 1, sizeof(char));
                    strcpy(command->args[0], command->command);

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

                    execvp(command->command, command->args);
                    perror("execvp");
                    exit(1);
                    break;
                default:
                    //Parent process
                    if (!command->f_background_process){
                            child_pid = waitpid(child_pid, &last_exit_status, 0);

                        if(WIFEXITED(last_exit_status)){
                            last_exit_status = WEXITSTATUS(last_exit_status);
                        }
                        else {
                            last_exit_status = WTERMSIG(last_exit_status);
                        }

                        // printf("Child exited with status %d\n", last_exit_status);
                    }
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

        // Print output for troubleshooting
        printf("Command: %s\n", command->command);
        for(int i=0; i<514; i++){
            if(command->args[i] != NULL){
                printf("Arg %d: %s\n", i, command->args[i]);
            }
        }
        if(command->f_input_file){
            printf("Input: %s\n", command->input_file);
        }
        if(command->f_output_file){
            printf("Output: %s\n", command->output_file);
        }
        if(command->f_background_process){
            printf("Background process\n");
        }

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