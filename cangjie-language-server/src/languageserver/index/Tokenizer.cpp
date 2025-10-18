// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "Tokenizer.h"
#include <algorithm>

namespace ark {
namespace lsp {
namespace {

inline constexpr bool IsUpper(char c) { return 'A' <= c && c <= 'Z'; }

/**
 * Checks if character C is a valid letter as classified by "C" locale.
 */
inline bool IsAlpha(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

/**
 * Checks if character C is one of the 10 decimal digits.
 */
inline bool IsDigit(char c) { return c >= '0' && c <= '9'; }

/**
 * Checks whether character C is either a decimal digit or an uppercase or
 * lowercase letter as classified by "C" locale.
 */
inline bool IsAlnum(char c) { return IsAlpha(c) || IsDigit(c); }

/**
 * Returns the corresponding lowercase character if x is uppercase.
 */
inline char ToLower(char x)
{
    if (x >= 'A' && x <= 'Z') {
        return x - 'A' + 'a';
    }
    return x;
}

size_t FindIfIsAlnum(std::string_view str)
{
    std::size_t pos = str.find_first_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    return pos;
}

static size_t FindIfIsNotAlnumFrom(std::string_view str, std::size_t start)
{
    std::string_view subStr = str.substr(start);
    std::size_t pos = subStr.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    if (pos != std::string::npos) {
        return pos + start;
    }
    return pos;
}

size_t FindIfIsNotUpper(std::string_view str)
{
    std::size_t pos = str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    return pos;
}

size_t FindIfIsUpperFrom(std::string_view str, std::size_t start)
{
    std::string_view subStr = str.substr(start);
    std::size_t pos = subStr.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    if (pos != std::string::npos) {
        return pos + start;
    }
    return pos;
}

std::pair<std::string, std::string> Split(std::string str)
{
    size_t start = FindIfIsAlnum(str);
    if (start != std::string::npos) {
        size_t end = FindIfIsNotAlnumFrom(str, start);
        if (end != std::string::npos) {
            std::string res = str.substr(start, end - start);
            return {res, str.substr(end)};
        } else {
            std::string res = str.substr(start);
            return {res, ""};
        }
    }
    return {};
}

struct IdenitifierTokenizer {
    void operator()(std::string_view orig_text, TokenizerCallback callback) const
    {
        std::string tokenBuffer;
        auto handleToken = [&](std::string token) {
            tokenBuffer.resize(token.size());
            std::transform(token.begin(), token.end(), tokenBuffer.begin(), ToLower);
            callback(tokenBuffer, token.size());
        };
        std::string text = orig_text.data();
        while (!text.empty()) {
            std::string fragment;
            std::tie(fragment, text) = Split(text);
            // Skip fragments containing numeric literals.
            if (std::all_of(fragment.begin(), fragment.end(), IsDigit)) {
                continue;
            }
            while (!fragment.empty()) {
                size_t pos = FindIfIsNotUpper(fragment);
                if (pos == std::string::npos) {
                    handleToken(fragment);
                    break;
                }
                if (pos > 1) {
                    handleToken(fragment.substr(0, pos - 1));
                    fragment = fragment.substr(pos - 1);
                    continue;
                }
                pos = FindIfIsUpperFrom(fragment, 1);
                if (pos == std::string::npos) {
                    handleToken(fragment);
                    break;
                }
                handleToken(fragment.substr(0, pos));
                fragment = fragment.substr(pos);
            }
        }
    }
};

} // namespace

TokenizerFunction GetIdentifierTokenizer() { return IdenitifierTokenizer(); }

} // namespace lsp
} // namespace ark

