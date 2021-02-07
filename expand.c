#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "expand.h"

/*
 * Expands $$ in the commandString to the process' ID.
 */
void expandVariable(char* commandString, char* finalString){
    char processIdStr[128];
    size_t bytes_to_write;
    int start_index = 0;
    int final_index = 0;

    // Get the PID and put it into string format
    pid_t processId = getpid();
    sprintf(processIdStr, "%d", (int)processId);

    // Find the substring $$, if possible
    char* saveptr = strstr(commandString, "$$");

    // Expand instances of $$
    while(saveptr != NULL){
        bytes_to_write = saveptr - (commandString + start_index);

        // First, copy over all the bytes up to the instance of $$
        strncpy(finalString + final_index, commandString + start_index, bytes_to_write);
        start_index += bytes_to_write;
        final_index += bytes_to_write;

        // Then, place the PID where the $$ characters were
        strncpy(finalString + final_index, processIdStr, strlen(processIdStr));
        final_index += strlen(processIdStr);

        // Move loop along by moving the indices we should be looking from
        start_index += 2;
        saveptr = strstr(commandString + start_index, "$$");
    }

    // Copy any bytes left after the $$, or all the bytes if $$ is not present.
    strncpy(finalString + final_index, commandString + start_index, strlen(commandString+start_index));

    return;
}