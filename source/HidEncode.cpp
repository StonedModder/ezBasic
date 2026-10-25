#include "HidEncode.hpp"

namespace ezbasic {

bool HidEncode(char c, HidStroke& out) {
    out.mods = 0;
    out.key = 0;

    unsigned char uc = static_cast<unsigned char>(c);

    if (uc >= 'a' && uc <= 'z') {
        out.key = 4 + (uc - 'a');
        out.mods = 2; // Left Shift — inverted shift for lowercase
        return true;
    }
    if (uc >= 'A' && uc <= 'Z') {
        out.key = 4 + (uc - 'A');
        return true;
    }
    if (uc >= '1' && uc <= '9') {
        out.key = 30 + (uc - '1');
        return true;
    }
    if (uc == '0') {
        out.key = 39;
        return true;
    }
    if (uc == '\n') {
        out.key = 40; // Enter/Return
        return true;
    }
    if (uc == '\t') {
        out.key = 43; // Tab
        return true;
    }
    if (uc == ' ') {
        out.key = 44; // Space
        return true;
    }

    const char* unshifted = "-=[]\\;',./`";
    const char* shifted   = "_+{}|:\"<>?~";
    for (int i = 0; unshifted[i]; ++i) {
        if (uc == static_cast<unsigned char>(unshifted[i]) ||
            uc == static_cast<unsigned char>(shifted[i])) {
            static const uint8_t codes[] = {45, 46, 47, 48, 49, 51, 52, 54, 55, 56, 53};
            out.key = codes[i];
            out.mods = (uc == static_cast<unsigned char>(shifted[i])) ? 2 : 0;
            return true;
        }
    }

    const char* shift_digits = "!@#$%^&*()";
    for (int i = 0; i < 10; ++i) {
        if (uc == static_cast<unsigned char>(shift_digits[i])) {
            out.key = (i == 9) ? 39 : (30 + i);
            out.mods = 2;
            return true;
        }
    }

    return false;
}

} // namespace ezbasic
