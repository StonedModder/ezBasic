#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ezbasic {

/**
 * Represents a file loaded from RomFS with its raw bytes, size, and SHA-256 hash.
 */
struct RomfsFile {
    std::vector<uint8_t> data;
    std::size_t size = 0;
    std::string sha256;
};

/**
 * Load a file from RomFS by path (e.g. "romfs:/bootstrap.bas").
 * Returns std::nullopt if the file cannot be opened or read.
 * The returned RomfsFile includes the raw bytes, size, and SHA-256 hex string.
 */
std::optional<RomfsFile> LoadRomfsFile(const std::string& path);

/**
 * Convenience helper: load a RomFS text file and return its contents as a UTF-8 string.
 * Returns std::nullopt on failure.
 */
std::optional<std::string> LoadRomfsText(const std::string& path);

/**
 * Strip leading comment lines (lines starting with ') from the text.
 * Also strips any blank lines immediately preceding the first code line.
 */
std::string StripLeadingComments(const std::string& text);

} // namespace ezbasic
