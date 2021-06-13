#pragma once
#include <cstdint>

struct KeyboardInput {
    uint8_t vk;
    uint8_t flags;  //down = 1
};

void DriverKeyboardSend(KeyboardInput inputs[], uint32_t n);