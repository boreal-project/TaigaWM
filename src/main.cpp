#include <limits.h>
#include <unistd.h>

#include <cstdlib>

#include "autostart.hpp"
#include "config.hpp"
#include "util.hpp"
#include "wm.hpp"

namespace {

int compositorMain() {
    if (!wm.connect()) {
        return 1;
    }
    if (config.autostart) {
        runAutostart(*config.autostart);
    }
    return wm.run();
}

} // namespace

int main() {
    loadConfig();

    if (std::getenv("TAIGA_WRAPPED") != nullptr) {
        return compositorMain();
    }

    char exePath[PATH_MAX];
    const ssize_t len =
        readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len <= 0) {
        diag::error("failed to read /proc/self/exe.");
        return 1;
    }
    exePath[len] = '\0';

    setenv("TAIGA_WRAPPED", "1", 1);
    char river[] = "river";
    char flag[] = "-c";
    char *const args[] = {river, flag, exePath, nullptr};
    execvp("river", args);

    perror("execvp");
    return 1;
}
