#include "RomfsBootstrap.hpp"
#include "Utils.hpp"

#include <cstdio>
#include <cstdlib>

namespace ezbasic {

std::optional<RomfsFile> LoadRomfsFile(const std::string& path) {
    std::FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        return std::nullopt;
    }

    // Determine file size
    if (std::fseek(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        return std::nullopt;
    }
    long sizeLong = std::ftell(fp);
    if (sizeLong < 0) {
        std::fclose(fp);
        return std::nullopt;
    }
    if (std::fseek(fp, 0, SEEK_SET) != 0) {
        std::fclose(fp);
        return std::nullopt;
    }

    std::size_t size = static_cast<std::size_t>(sizeLong);
    std::vector<uint8_t> buffer(size);
    if (size > 0) {
        std::size_t read = std::fread(buffer.data(), 1, size, fp);
        if (read != size) {
            std::fclose(fp);
            return std::nullopt;
        }
    }
    std::fclose(fp);

    RomfsFile result;
    result.data = std::move(buffer);
    result.size = size;
    result.sha256 = Sha256ToString(CalculateSha256(result.data.data(), result.size));
    return result;
}

std::string StripLeadingComments(const std::string& text) {
    std::size_t pos = 0;
    while (pos < text.size()) {
        // Find the end of the current line
        std::size_t lineEnd = text.find('\n', pos);
        if (lineEnd == std::string::npos) {
            lineEnd = text.size();
        }
        
        // Check if this line is a comment line (starts with ' after optional whitespace)
        std::size_t firstNonSpace = pos;
        while (firstNonSpace < lineEnd && 
               (text[firstNonSpace] == ' ' || text[firstNonSpace] == '\t' || text[firstNonSpace] == '\r')) {
            firstNonSpace++;
        }
        
        // If the line starts with ', it's a comment — skip it
        if (firstNonSpace < lineEnd && text[firstNonSpace] == '\'') {
            pos = lineEnd + 1;
            continue;
        }
        
        // If the line is blank (only whitespace), skip it too
        if (firstNonSpace == lineEnd) {
            pos = lineEnd + 1;
            continue;
        }
        
        // Found the first non-comment, non-blank line
        break;
    }
    
    return text.substr(pos);
}

std::optional<std::string> LoadRomfsText(const std::string& path) {
    auto file = LoadRomfsFile(path);
    if (!file) {
        return std::nullopt;
    }
    std::string raw(reinterpret_cast<const char*>(file->data.data()), file->size);
    return StripLeadingComments(raw);
}

} // namespace ezbasic
