// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include <algorithm>
#include <cangjie/Utils/FileUtil.h>
#include <fstream>
#include <gtest/gtest.h>
#include <vector>
#include "../../../src/languageserver/common/Utils.h"

using namespace Cangjie::FileUtil;

namespace apitest {

    class GetAllFilesTest : public ::testing::Test {
    protected:
        void SetUp() override
        {
            CreateDirs("test_dir/");
            std::ofstream("test_dir/file.cj").close();
            std::ofstream("test_dir/file_test.cj").close();
        }

        void TearDown() override
        {
            Remove("test_dir/file.cj");
            Remove("test_dir/file_test.cj");
            Remove("test_dir");
        }
    };

    TEST_F(GetAllFilesTest, FindsCjFiles)
    {
        auto result = GetAllFilesUnderCurrentPath("test_dir", "cj");
        EXPECT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], "file.cj");
    }

    TEST_F(GetAllFilesTest, IncludesTestFiles)
    {
        auto result = GetAllFilesUnderCurrentPath("test_dir", "cj", false);
        EXPECT_EQ(result.size(), 2);
        EXPECT_TRUE(std::find(result.begin(), result.end(), "file_test.cj") != result.end());
    }

    class StringMatcherTest : public ::testing::Test {
    protected:
        void SetUp() override {}

        void TearDown() override {}
    };

    TEST_F(StringMatcherTest, CaseSensitiveMatching)
    {
        // 测试完全匹配
        EXPECT_TRUE(ark::IsMatchingCompletion("Hld", "HelloWorld", true));

        // 测试大小写不匹配的情况
        EXPECT_FALSE(ark::IsMatchingCompletion("hw", "HelloWorld", true));

        // 测试字符顺序
        EXPECT_TRUE(ark::IsMatchingCompletion("HW", "HelloWorld", true));
        EXPECT_FALSE(ark::IsMatchingCompletion("WH", "HelloWorld", true));

        // 测试不存在的字符
        EXPECT_FALSE(ark::IsMatchingCompletion("xyz", "HelloWorld", true));
    }

    TEST_F(StringMatcherTest, CaseInsensitiveMatching)
    {
        // 测试不同大小写的匹配
        EXPECT_TRUE(ark::IsMatchingCompletion("hw", "HelloWorld", false));
        EXPECT_TRUE(ark::IsMatchingCompletion("HELLO", "HelloWorld", false));
        EXPECT_TRUE(ark::IsMatchingCompletion("hElLo", "HelloWorld", false));

        // 测试字符顺序
        EXPECT_TRUE(ark::IsMatchingCompletion("hw", "HelloWorld", false));
        EXPECT_FALSE(ark::IsMatchingCompletion("wh", "HelloWorld", false));

        // 测试不存在的字符
        EXPECT_FALSE(ark::IsMatchingCompletion("xyz", "HelloWorld", false));
        // 测试混合大小写
        EXPECT_TRUE(ark::IsMatchingCompletion("hW", "HelloWorld", false));
        EXPECT_TRUE(ark::IsMatchingCompletion("Hw", "HelloWorld", false));
    }

}