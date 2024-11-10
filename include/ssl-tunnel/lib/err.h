#pragma once

#include <stdarg.h> // va_list
#include <string.h> // strcmp

typedef struct {
    char *msg;
} err_t;

#define ERROR_OK(err) ((err).msg == NULL)
#define ERROR_IS(err1, err2) (strcmp((err1).msg, (err2).msg) == 0)

#define ENULL (err_t){ .msg = NULL }

err_t err_errno();

void panicf(const char *msg, ...);

void vpanicf(const char *msg, va_list va);