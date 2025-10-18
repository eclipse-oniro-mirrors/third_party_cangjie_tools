// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "Format/TomlParser.h"
#include <limits>

using namespace Cangjie::Format;

static const int BEGIN_INDEX_OF_STRING = 1;
static const int END_INDEX_OF_STRING = 2;

bool TomlParser::ReadFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        // skip empty line
        if (line.empty()) {
            continue;
        }

        // remove comments
        auto commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos); // retain the part before the comment
        }

        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = Trim(line.substr(0, pos));
            std::string value = Trim(line.substr(pos + 1));
            data[key] = ParseValue(value);
        }
    }

    file.close();
    return true;
}

std::optional<TomlParser::ValueType> TomlParser::GetValue(const std::string& key) const
{
    auto it = data.find(key);
    if (it != data.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string TomlParser::Trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t");
    size_t last = str.find_last_not_of(" \t");
    return (first == std::string::npos) ? "" : str.substr(first, (last - first + 1));
}

std::optional<TomlParser::ValueType> TomlParser::ParseValue(const std::string& value)
{
    std::string trimmedValue = Trim(value);
    // Process a Boolean value.
    if (trimmedValue == "true") {
        return true;
    }
    if (trimmedValue == "false") {
        return false;
    }

    // Process integers.
    if (!trimmedValue.empty() &&
        (std::isdigit(trimmedValue[0]) || trimmedValue[0] == '-' || trimmedValue[0] == '+')) {
        size_t idx;
        long intValue = std::stol(trimmedValue, &idx);
        // Check if the entire string was converted and ensure it is within the int range
        if (idx == trimmedValue.size() && intValue >= std::numeric_limits<int>::min() &&
            intValue <= std::numeric_limits<int>::max()) {
            return static_cast<int>(intValue);
        }
    }

    // Process the string (remove the quotes)
    if (trimmedValue.front() == '"' && trimmedValue.back() == '"') {
        // remove the quotes
        return trimmedValue.substr(BEGIN_INDEX_OF_STRING, trimmedValue.size() - END_INDEX_OF_STRING);
    }
    return std::nullopt;
}