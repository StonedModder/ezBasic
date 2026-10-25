#pragma once
#include <cstdint>

namespace ezbasic {

struct HidStroke {
    uint8_t key = 0;
    uint8_t mods = 0;
};

bool HidEncode(char c, HidStroke& out);

} // namespace ezbasic
