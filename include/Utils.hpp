#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ezbasic {

/**
 * Calculate the SHA-256 hash of a data buffer.
 * Portable implementation; does not depend on libnx crypto services.
 */
std::vector<uint8_t> CalculateSha256(const void* data, size_t size);

/**
 * Convert a SHA-256 byte vector to a lowercase hex string.
 */
std::string Sha256ToString(const std::vector<uint8_t>& hash);

/**
 * Format seconds as MM:SS (e.g. 73.5 -> "01:13").
 */
std::string SecondsToMMSS(double seconds);

/**
 * Format a percentage: value / total * 100 with one decimal place.
 * Returns "0.0%" if total is zero.
 */
std::string FormatPercentage(double value, double total);

} // namespace ezbasic
