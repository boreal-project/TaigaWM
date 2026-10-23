#pragma once

#include <unistd.h>

namespace diag {

void info(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void warn(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

} // namespace diag

inline constexpr auto noop = [](auto...) {};

class UniqueFd {
  public:
    explicit UniqueFd(int fd = -1) : fd_(fd) {}
    ~UniqueFd() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }
    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;

    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }

  private:
    int fd_;
};
