// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include <gtest/gtest.h>
#include "BasicHelper.cpp"
#include <string>
#include <functional>

using namespace ark;

// CountLeadingOnes test
TEST(BasicHelperTest, CountLeadingOnes) {
    EXPECT_EQ(0, CountLeadingOnes(0x00));  // 00000000
    EXPECT_EQ(1, CountLeadingOnes(0x80));  // 10000000
    EXPECT_EQ(2, CountLeadingOnes(0xC0));  // 11000000
    EXPECT_EQ(3, CountLeadingOnes(0xE0));  // 11100000
    EXPECT_EQ(4, CountLeadingOnes(0xF0));  // 11110000
    EXPECT_EQ(5, CountLeadingOnes(0xF8));  // 11111000
    EXPECT_EQ(6, CountLeadingOnes(0xFC));  // 11111100
    EXPECT_EQ(7, CountLeadingOnes(0xFE));  // 11111110
    EXPECT_EQ(8, CountLeadingOnes(0xFF));  // 11111111
}

// IsProxyPairs test
TEST(BasicHelperTest, IsProxyPairs_ASCII) {
    std::string asciiStr = "A";
    size_t index = 0;
    EXPECT_FALSE(IsProxyPairs(asciiStr, index));
}

TEST(BasicHelperTest, IsProxyPairs_TwoByte) {
    std::string twoByteStr = "£";  // 0xC2 0xA3
    size_t index = 0;
    EXPECT_FALSE(IsProxyPairs(twoByteStr, index));
}

TEST(BasicHelperTest, IsProxyPairs_ThreeByte) {
    std::string threeByteStr = "中";  // 0xE4 0xB8 0xAD
    size_t index = 0;
    EXPECT_FALSE(IsProxyPairs(threeByteStr, index));
}

TEST(BasicHelperTest, IsProxyPairs_FourByteProxyPair) {
    std::string fourByteStr = "😊";  // 0xF0 0x9F 0x98 0x80 (codepoint > 0xFFFF)
    size_t index = 0;
    EXPECT_TRUE(IsProxyPairs(fourByteStr, index));
}

// IterCodePointsOfUTF8 test
TEST(BasicHelperTest, IterCodePointsOfUTF8_EmptyString) {
    EXPECT_TRUE(IterCodePointsOfUTF8("", [](int len, int units) {
        return false;
    }));
}

TEST(BasicHelperTest, IterCodePointsOfUTF8_ASCIIOnly) {
    std::string asciiStr = "Hello";
    int count = 0;
    int totalLength = 0;
    int totalUnits = 0;

    IterCodePointsOfUTF8(asciiStr, [&count, &totalLength, &totalUnits](int len, int units) {
        count++;
        totalLength += len;
        totalUnits += units;
        return false;
    });

    EXPECT_EQ(5, count);      // 5 characters
    EXPECT_EQ(5, totalLength); // Total length 5 bytes
    EXPECT_EQ(5, totalUnits);  // Total 5 units
}

TEST(BasicHelperTest, IterCodePointsOfUTF8_MixedCharacters) {
    std::string mixedStr = "H中😊";  // H(1 byte) + 中(3 bytes) + 😊(4 bytes, surrogate pair)
    std::vector<std::pair<int, int>> results;

    IterCodePointsOfUTF8(mixedStr, [&results](int len, int units) {
        results.push_back({len, units});
        return false;
    });

    ASSERT_EQ(3, results.size());
    EXPECT_EQ(1, results[0].first);   // H: 1 byte
    EXPECT_EQ(1, results[0].second);  // H: 1 unit
    EXPECT_EQ(3, results[1].first);   // 中: 3 bytes
    EXPECT_EQ(1, results[1].second);  // 中: 1 unit
    EXPECT_EQ(4, results[2].first);   // 😊: 4 bytes
    EXPECT_EQ(2, results[2].second);  // 😊: 2 units (surrogate pair)
}

TEST(BasicHelperTest, IterCodePointsOfUTF8_CallbackEarlyReturn) {
    std::string testStr = "Hello";
    int callCount = 0;

    bool result = IterCodePointsOfUTF8(testStr, [&callCount](int len, int units) {
        callCount++;
        return callCount >= 3; // Return true after processing 3 characters
    });

    EXPECT_TRUE(result);     // Should return early
    EXPECT_EQ(3, callCount); // Should be called only 3 times
}

// MeasureUnits test
TEST(BasicHelperTest, MeasureUnits_NegativeColumn) {
    bool valid = true;
    EXPECT_EQ(0, MeasureUnits("test", -1, OffsetEncoding::UTF8, valid));
    EXPECT_FALSE(valid);
}

TEST(BasicHelperTest, MeasureUnits_EmptyString_ZeroColumn) {
    bool valid = true;
    EXPECT_EQ(0, MeasureUnits("", 0, OffsetEncoding::UTF8, valid));
    EXPECT_TRUE(valid);
}

TEST(BasicHelperTest, MeasureUnits_ASCIIString) {
    bool valid = true;
    EXPECT_EQ(4, MeasureUnits("test", 4, OffsetEncoding::UTF8, valid));
    EXPECT_TRUE(valid);
}

TEST(BasicHelperTest, MeasureUnits_UTF8String) {
    bool valid = true;
    EXPECT_EQ(3, MeasureUnits("中", 1, OffsetEncoding::UTF8, valid)); // "中" is 3 bytes
    EXPECT_TRUE(valid);
}

TEST(BasicHelperTest, MeasureUnits_MixedString) {
    bool valid = true;
    // "H中" = H(1 byte) + 中(3 bytes) = 4 bytes total
    EXPECT_EQ(4, MeasureUnits("H中", 2, OffsetEncoding::UTF8, valid));
    EXPECT_TRUE(valid);
}

TEST(BasicHelperTest, MeasureUnits_ColumnOutOfRange) {
    bool valid = true;
    EXPECT_EQ(5, MeasureUnits("Hello", 10, OffsetEncoding::UTF8, valid)); // Should return entire string length
    EXPECT_FALSE(valid); // Out of range should be invalid
}

TEST(BasicHelperTest, MeasureUnits_UnsupportedEncoding) {
    bool valid = true;
    EXPECT_EQ(0, MeasureUnits("test", 2, OffsetEncoding::UTF16, valid));
    EXPECT_FALSE(valid);

    valid = true;
    EXPECT_EQ(0, MeasureUnits("test", 2, OffsetEncoding::UTF32, valid));
    EXPECT_FALSE(valid);
}

// GetOffsetFromPosition test
TEST(BasicHelperTest, GetOffsetFromPosition_NegativeLine) {
    ark::Position negLinePos(-1, 0);
    EXPECT_EQ(-1, GetOffsetFromPosition("test", negLinePos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_NegativeColumn) {
    ark::Position negColPos(0, -1);
    EXPECT_EQ(-1, GetOffsetFromPosition("test", negColPos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_SingleLine_ValidPosition) {
    ark::Position singleLinePos(0, 2);
    EXPECT_EQ(2, GetOffsetFromPosition("hello", singleLinePos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_MultiLine_ValidPosition) {
    std::string multiLineText = "hello\nworld\n";
    ark::Position multiLinePos(1, 2);
    // Line 0: "hello\n" (6 characters)
    // Line 1: starting from index 6, "wo" = 2 character offset
    // Total offset = 6 + 2 = 8
    EXPECT_EQ(8, GetOffsetFromPosition(multiLineText, multiLinePos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_LastLineWithoutNewline) {
    std::string text = "line1\nline2";
    ark::Position pos(1, 2); // 3rd character ('n') on second line
    // Line 0: "line1\n" (6 characters)
    // Line 1: starting from index 6, "li" = 2 character offset
    // Total offset = 6 + 2 = 8
    EXPECT_EQ(8, GetOffsetFromPosition(text, pos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_LineOutOfRange) {
    std::string text = "line1\nline2\n";
    ark::Position outOfRangePos(5, 0); // Non-existent line
    EXPECT_EQ(-1, GetOffsetFromPosition(text, outOfRangePos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_ColumnOutOfRange) {
    std::string text = "short";
    ark::Position colOutOfRangePos(0, 100); // Column out of range
    EXPECT_EQ(-1, GetOffsetFromPosition(text, colOutOfRangePos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_UTF8Characters) {
    std::string text = "H中😊\nworld";
    ark::Position pos(0, 2); // 3rd character ('中') on first line
    // H(1 byte) + 中(3 bytes) = 4 byte offset
    EXPECT_EQ(4, GetOffsetFromPosition(text, pos));
}

TEST(BasicHelperTest, GetOffsetFromPosition_UTF8ProxyPairs) {
    std::string text = "H中😊\nworld";
    ark::Position pos(0, 3); // 4th character ('😊', surrogate pair takes 2 units) on first line
    // H(1 byte) + 中(3 bytes) + 😊(4 bytes) = 8 byte offset
    EXPECT_EQ(4, GetOffsetFromPosition(text, pos));
}
