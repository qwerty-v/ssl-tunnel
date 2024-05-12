#pragma once

typedef int err_t;

extern const err_t ERROR_OK;

void errors_print(err_t err);

#define ERR_OK(expr) ((expr) == ERROR_OK)
