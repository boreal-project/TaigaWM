#include "util.hpp"

#include <cstdarg>
#include <cstdio>

namespace {

void emit(FILE *stream, const char *prefix, const char *fmt, va_list args) {
    std::fputs(prefix, stream);
    std::vfprintf(stream, fmt, args);
    std::fputc('\n', stream);
}

} // namespace

namespace diag {

void info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    emit(stdout, "", fmt, args);
    va_end(args);
}

void warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    emit(stderr, "warn: ", fmt, args);
    va_end(args);
}

void error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    emit(stderr, "err: ", fmt, args);
    va_end(args);
}

} // namespace diag
