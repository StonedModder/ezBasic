#include "HidEncode.hpp"
#include "RomfsBootstrap.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    assert(argc == 2);

    const std::string sample = "' header one\r\n  \t\r\n  ' header two\n\nVAR X=1\r\n' keep later comments\n";
    const std::string stripped = ezbasic::StripLeadingComments(sample);
    assert(stripped == "VAR X=1\r\n' keep later comments\n");
    assert(ezbasic::StripLeadingComments("' only comment\n\n") == "");
    assert(ezbasic::StripLeadingComments("VAR X=1\n' not leading\n") == "VAR X=1\n' not leading\n");

    const auto text = ezbasic::LoadRomfsText(argv[1]);
    assert(text.has_value());
    assert(!text->empty());
    assert(text->rfind("VAR NSR", 0) == 0);

    std::size_t strokes = 0;
    std::size_t lines = 0;
    for (std::size_t i = 0; i < text->size(); ++i) {
        if ((*text)[i] == '\r' && i + 1 < text->size() && (*text)[i + 1] == '\n') continue;
        ezbasic::HidStroke stroke;
        if (!ezbasic::HidEncode((*text)[i], stroke)) {
            std::cerr << "untypable byte at offset " << i << ": 0x"
                      << std::hex << static_cast<unsigned int>(static_cast<unsigned char>((*text)[i]))
                      << '\n';
            return 1;
        }
        ++strokes;
        if ((*text)[i] == '\n') ++lines;
    }

    std::cout << "asset smoke test passed; bytes=" << text->size()
              << " strokes=" << strokes << " lines=" << lines << '\n';
    return 0;
}
