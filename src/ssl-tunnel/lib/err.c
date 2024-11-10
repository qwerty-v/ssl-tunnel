#include <ssl-tunnel/lib/err.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h> // strerror
#include <errno.h> // errno

err_t err_errno() {
    return (err_t) {
            .msg = strerror(errno)
    };
}

void panicf(const char *msg, ...) {
    va_list va;
    va_start(va, msg);
    vpanicf(msg, va);
    va_end(va);
}

void vpanicf(const char *msg, va_list va) {
    vfprintf(stderr, msg, va);
    fprintf(stderr, "\n");
    abort();
}