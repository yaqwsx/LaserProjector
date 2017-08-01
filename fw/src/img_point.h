#pragma once

#include <cstdint>

struct __attribute__((packed)) ImgPoint {
    int16_t x, y;
    uint8_t l;
};