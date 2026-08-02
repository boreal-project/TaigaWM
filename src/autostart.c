#include <stdlib.h>
#include <unistd.h>

#include "autostart.h"

void autostart(char **autostart_list, size_t autostart_list_sz) {
    fprintf(stdout, "autostart\n");
    for (size_t i = 0; i < autostart_list_sz; i++) {
        if (fork() == 0) {
            execl("/bin/sh", "/bin/sh", "-c", autostart_list[i], NULL);
            _exit(127);
        }
        free(autostart_list[i]);
    }
    free(autostart_list);
}
