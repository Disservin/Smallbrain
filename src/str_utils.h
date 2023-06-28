#pragma once

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace str_util {
[[nodiscard]] inline bool startsWith(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return false;
    return (haystack.rfind(needle, 0) != std::string::npos);
}

[[nodiscard]] inline bool endsWith(std::string_view value, std::string_view ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

[[nodiscard]] inline std::string toLower(std::string_view string) {
    std::string lower_string(string);
    std::transform(lower_string.begin(), lower_string.end(), lower_string.begin(), ::tolower);
    return lower_string;
}

[[nodiscard]] inline bool contains(std::string_view haystack, std::string_view needle) {
    return haystack.find(needle) != std::string::npos;
}

[[nodiscard]] inline bool contains(const std::vector<std::string> &haystack,
                                   std::string_view needle) {
    return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

[[nodiscard]] inline std::vector<std::string> splitString(const std::string &string,
                                                          const char &delimiter) {
    std::stringstream string_stream(string);
    std::string segment;
    std::vector<std::string> seglist;

    while (std::getline(string_stream, segment, delimiter)) seglist.emplace_back(segment);

    return seglist;
}

template <typename T>
[[nodiscard]] std::optional<T> findElement(const std::vector<std::string> &haystack,
                                           std::string_view needle) {
    auto position = std::find(haystack.begin(), haystack.end(), needle);
    auto index = position - haystack.begin();
    if (position == haystack.end()) return std::nullopt;

    if constexpr (std::is_same_v<T, int>)
        return std::stoi(haystack[index + 1]);
    else if constexpr (std::is_same_v<T, float>)
        return std::stof(haystack[index + 1]);
    else if constexpr (std::is_same_v<T, U64>)
        return std::stoull(haystack[index + 1]);
    else if constexpr (std::is_same_v<T, int64_t>)
        return std::stoll(haystack[index + 1]);
    else if constexpr (std::is_same_v<T, bool>)
        return haystack[index + 1] == "true";
    else if (size_t(index + 1) < haystack.size())
        return haystack[index + 1];

    return std::nullopt;
}

[[nodiscard]] inline int64_t findRange(const std::string &string, const std::string &start,
                                       const std::string &end) {
    const auto start_pos = string.find(start);
    const auto end_pos = string.find(end);

    if (start_pos == std::string::npos || end_pos == std::string::npos) return -1;

    const auto start_pos_offset = start_pos + start.length();

    return end_pos - start_pos_offset - 1;
}

static inline void ltrim(std::string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

}  // namespace str_util