#pragma once

#include <stdbool.h>

#define optional_t(T)   \
    struct {            \
        bool present;   \
        T value;        \
    }
