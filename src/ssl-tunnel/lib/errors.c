#include <ssl-tunnel/lib/errors.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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