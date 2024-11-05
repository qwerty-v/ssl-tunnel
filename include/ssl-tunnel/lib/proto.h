#pragma once

#include <stdint.h>

#define MAX_MTU 1500

// proto_wire_t is an unparsed protocol packet captured on the wire.
// This type guarantees to be able to hold any such packet
typedef struct {
    uint8_t data[MAX_MTU];
} proto_wire_t;

typedef struct {
} proto_header_t;