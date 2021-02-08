#include <string.h>

/* struct for background process information */
struct BgProcess
{
    pid_t bg_pid;
    pid_t bg_done;
    int bg_status;
    struct BgProcess* next;
};