// If you are not compiling with the gcc option --std=gnu99, then
// uncomment the following line or you might get a compiler warning
//#define _GNU_SOURCE
#define STDIN 0
#define STDOUT 1
#define STDERR 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellcommand.h"
#include "parsecommand.h"
#include "expand.h"

/* Parse the current line which is space delimited and create a
*  student struct with the data in this line
*/
// struct student *createStudent(char *currLine)
// {
//     struct student *currStudent = malloc(sizeof(struct student));

//     // For use with strtok_r
//     char *saveptr;

//     // The first token is the ONID
//     char *token = strtok_r(currLine, " ", &saveptr);
//     currStudent->onid = calloc(strlen(token) + 1, sizeof(char));
//     strcpy(currStudent->onid, token);

//     // The next token is the lastName
//     token = strtok_r(NULL, " ", &saveptr);
//     currStudent->lastName = calloc(strlen(token) + 1, sizeof(char));
//     strcpy(currStudent->lastName, token);

//     // The next token is the firstName
//     token = strtok_r(NULL, " ", &saveptr);
//     currStudent->firstName = calloc(strlen(token) + 1, sizeof(char));
//     strcpy(currStudent->firstName, token);

//     // The last token is the major
//     token = strtok_r(NULL, "\n", &saveptr);
//     currStudent->major = calloc(strlen(token) + 1, sizeof(char));
//     strcpy(currStudent->major, token);

//     // Set the next node to NULL in the newly created student entry
//     currStudent->next = NULL;

//     return currStudent;
// }

/*
* Return a linked list of students by parsing data from
* each line of the specified file.
*/
// struct student *processFile(char *filePath)
// {
//     // Open the specified file for reading only
//     FILE *studentFile = fopen(filePath, "r");

//     char *currLine = NULL;
//     size_t len = 0;
//     ssize_t nread;
//     char *token;

//     // The head of the linked list
//     struct student *head = NULL;
//     // The tail of the linked list
//     struct student *tail = NULL;

//     // Read the file line by line
//     while ((nread = getline(&currLine, &len, studentFile)) != -1)
//     {
//         // Get a new student node corresponding to the current line
//         struct student *newNode = createStudent(currLine);

//         // Is this the first node in the linked list?
//         if (head == NULL)
//         {
//             // This is the first node in the linked link
//             // Set the head and the tail to this node
//             head = newNode;
//             tail = newNode;
//         }
//         else
//         {
//             // This is not the first node.
//             // Add this node to the list and advance the tail
//             tail->next = newNode;
//             tail = newNode;
//         }
//     }
//     free(currLine);
//     fclose(studentFile);
//     return head;
// }

int main(int argc, char *argv[])
{
    // char * startLine = ":"
    char* readBuffer = calloc(sizeof(char), 2048);
    char* expandedString = calloc(sizeof(char), 2048);
    struct ShellCommand* command;

    // Put semicolon on new line
    printf(": ");
    fflush(stdout);

    // Read in user input
    scanf("%[^\n]s", readBuffer);
    fflush(stdin);

    // Expand the string if necessary
    expandVariable(readBuffer, expandedString);

    // Parse the string into the ShellCommand struct
    command = parseCommand(expandedString);

    // Print output for troubleshooting
    printf("Parsed command\n");
    printf("Command: %s\n", command->command);
    for(int i=0; i<512; i++){
        if(strcmp(command->args[i], "") != 0){
            printf("Arg: %s\n", command->args[i]);
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
    free(command->command);
    if(command->f_input_file){
        free(command->input_file);
    }
    if(command->f_output_file){
        free(command->output_file);
    }
    free(command);
    free(readBuffer);

    return EXIT_SUCCESS;
}