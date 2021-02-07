#include <stdlib.h>
#include <stdio.h>
#include "parsecommand.h"
#include "shellcommand.h"

/*
 * This command parses the command string from the user into the ShellCommand struct (shellcommand.h).
 */
struct ShellCommand* parseCommand(char* commandString){
    // For use with strtok_r
    char* saveptr;
    struct ShellCommand* command = malloc(sizeof(struct ShellCommand));
    int f_command = 1;
    int arg_counter = 0;

    // Grab the background process char before strtok-ing the string
    int f_background = (commandString[strlen(commandString) - 1] == '&');

    // If the commandString begins with #, it is a comment
    // Don't do anything.
    if(commandString[0] == '#'){
        return command;
    }

    char* token = strtok_r(commandString, " ", &saveptr);

    while (token != NULL){
        // First token is the actual command
        // Process into the ShellCommand struct
        if (f_command){
            command->command = calloc(strlen(token) + 1, sizeof(char));
            memset(command->command, '\0', strlen(command->command));
            strcpy(command->command, token);
            f_command = 0;
        }
        // Get input from a file
        // Set the input fields in the ShellCommand struct
        else if(strcmp(token, "<") == 0){
            token = strtok_r(NULL, " ", &saveptr);

            // Is there a filename entered after the '<' symbol?
            if (token != NULL){
                command->input_file = calloc(strlen(token) + 1, sizeof(char));
                strcpy(command->input_file, token);
                command->f_input_file = 1;
            } else {
                // Assignment says there's no error handling the command line, so...
            }

        }
        // Output to specified file
        // Set the output files in the ShellCommand struct
        else if(strcmp(token, ">") == 0){
            token = strtok_r(NULL, " ", &saveptr);

            // Is there a filename entered after the '>' symbol?
            if (token != NULL){
                command->output_file = calloc(strlen(token) + 1, sizeof(char));
                strcpy(command->output_file, token);
                command->f_output_file = 1;
            } else {
                // Assignment says there's no error handling the command line, so...
            }
        }
        // Background process symbol
        // Need to test this. Not sure the second strcmp makes sense.
        else if((strcmp(token, "&") == 0) && f_background){
            command->f_background_process = 1;
        }
        // Otherwise it's an argument
        // Add to the argument array
        else{
            strcpy(command->args[arg_counter], token);
            arg_counter++;
            command->f_args = 1;
        }

        // Progress through the string
        token = strtok_r(NULL, " ", &saveptr);
    }

    return command;
}