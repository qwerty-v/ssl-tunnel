#pragma once

#include <stdbool.h> // bool

#define optional_t(T) \
struct {              \
    bool present;     \
    T v;              \
}

#define optional_some(vv) { .present = true, .v = vv }
#define optional_none() { .present = false }

#define optional_is_some(p) p.present
#define optional_is_none(p) !p.present

#define optional_unwrap(p) p.v
