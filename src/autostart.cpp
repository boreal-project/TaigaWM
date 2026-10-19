#include "autostart.hpp"

#include <unistd.h>

void spawnShell(const std::string &command) {
    if (fork() == 0) {
        execl("/bin/sh", "/bin/sh", "-c", command.c_str(), nullptr);
        _exit(127);
    }
}

void runAutostart(const std::vector<std::string> &commands) {
    for (const std::string &command : commands) {
        spawnShell(command);
    }
}
