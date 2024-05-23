#pragma once

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

typedef struct {
    bool ok;
    char *msg;
} err_t;

extern const err_t ERROR_OK;

#define ERR_OK(err) ((err).ok)
#define ERR_IS(err1, err2) (strcmp((err1).msg, (err2).msg) == 0)

void panicf(const char *msg, ...);
void vpanicf(const char *msg, va_list va);