#pragma once

#include <stdbool.h>

typedef struct {
    bool ok;
    char *msg;
} err_t;

extern const err_t ERROR_OK;
extern const err_t ERROR_OUT_OF_MEMORY;

#define ERR_OK(err) ((err).ok)