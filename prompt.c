#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "prompt.h"

void getInputFromPrompt(char* readBuffer, size_t* bufferSize){
    // Put semicolon on new line
    printf(": ");
    fflush(stdout);

    // Read in user input
    int numChars = getline(&readBuffer, bufferSize, stdin);
    if(numChars == -1){
        clearerr(stdin);
        strcpy(readBuffer, "");
    }
    else{
        // Remove newline char
        // Taken from the stackoverflow forum: https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
        char* newlinePos;
        if ((newlinePos = strchr(readBuffer, '\n')) != NULL){
            *newlinePos = '\0';
        }
    }

    fflush(stdin);
    return;
}