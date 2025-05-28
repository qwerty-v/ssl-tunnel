#pragma once

#include <stdbool.h> // bool

#define optional_t(T) \
struct {              \
    bool present;     \
    T v;              \
}

#define optional_arr_t(T, n) \
struct {                     \
    bool present;            \
    T v[n];                  \
}

#define optional_is_some(p) p.present
#define optional_is_none(p) !p.present

#define optional_unwrap(p) p.v
