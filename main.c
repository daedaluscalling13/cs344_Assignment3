// If you are not compiling with the gcc option --std=gnu99, then
// uncomment the following line or you might get a compiler warning
//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "shellcommand.h"
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

    // Starting cwd
    char current_working_directory [FILENAME_MAX];
    getcwd(current_working_directory, FILENAME_MAX);

    while(f_running){
        // Clear this string
        memset(expandedString, '\0', sizeof(expandedString));

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

        // DO COMMAND
        if (strcmp(command->command, "cd") == 0){
            if (strcmp(command->args[0], "") == 0){
                chdir(getenv("HOME"));
                getcwd(current_working_directory, FILENAME_MAX);
            }
            else{
                int chworks = chdir(command->args[0]);
                if (chworks == 0){
                    getcwd(current_working_directory, FILENAME_MAX);
                }
                else {
                    //No error handling apparently
                }
            }

        }
        else if (strcmp(command->command, "status") == 0){
            printf("exit value %d\n", last_exit_status);
            fflush(stdout);
        }
        else if (strcmp(command->command, "exit") == 0){
            //TODO: Kill all processes and jobs
            f_running = 0;
        }
        else {
            // Do other commands
        }

        // Print output for troubleshooting
        printf("Command: %s\n", command->command);
        for(int i=0; i<512; i++){
            if(command->f_args && strcmp(command->args[i], "") != 0){
                printf("Arg: %s\n", command->args[i]);
            }
        }
        // if(command->f_input_file){
        //     printf("Input: %s\n", command->input_file);
        // }
        // if(command->f_output_file){
        //     printf("Output: %s\n", command->output_file);
        // }
        // if(command->f_background_process){
        //     printf("Background process\n");
        // }

        // Free memory
        memset(command->command, '\0', sizeof(command->command));
        free(command->command);

        if(command->f_input_file){
            memset(command->input_file, '\0', sizeof(command->input_file));
            free(command->input_file);
        }
        if(command->f_output_file){
            memset(command->output_file, '\0', sizeof(command->output_file));
            free(command->output_file);
        }

        memset(command->args, '\0', 512*256);

        memset(command, '\0', sizeof(struct ShellCommand));
        free(command);

    }

    // Free local memory
    free(readBuffer);

    return EXIT_SUCCESS;
}