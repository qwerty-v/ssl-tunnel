#include <ssl-tunnel/errors.h>

const err_t ERROR_OK = {
        .ok = true,
};

const err_t ERROR_OUT_OF_MEMORY = {
        .ok = false,
        .msg = "out of memory",
};