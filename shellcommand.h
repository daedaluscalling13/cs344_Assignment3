#include <string.h>

/* struct for shell command information */
struct ShellCommand
{
    // Strings that specify the command
    char *command;
    char args[512][256];
    char *input_file;
    char *output_file;
    // Flags informing process functionality
    int f_args;
    int f_output_file;
    int f_input_file;
    int f_background_process;
};