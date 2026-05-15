#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

/**
 * Utility functions for string processing and logging
 */
namespace Utils {

/**
 * Convert string to lowercase
 */
std::string to_lowercase(const std::string& str);

/**
 * Trim whitespace from string
 */
std::string trim(const std::string& str);

/**
 * Split string by delimiter
 */
std::vector<std::string> split(const std::string& str, char delimiter);

/**
 * Check if string starts with prefix
 */
bool starts_with(const std::string& str, const std::string& prefix);

/**
 * Check if string ends with suffix
 */
bool ends_with(const std::string& str, const std::string& suffix);

/**
 * Replace all occurrences of pattern with replacement
 */
std::string replace_all(std::string str, const std::string& from, 
                        const std::string& to);

/**
 * URL-encode string
 */
std::string url_encode(const std::string& str);

/**
 * URL-decode string
 */
std::string url_decode(const std::string& str);

/**
 * Get current timestamp as string
 */
std::string get_timestamp();

/**
 * Format size in bytes as human-readable string
 */
std::string format_size(size_t bytes);

}  // namespace Utils

#endif // UTILS_H
