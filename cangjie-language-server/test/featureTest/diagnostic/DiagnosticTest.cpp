// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include<string>
#include<vector>

#include "gtest/gtest.h"
#include<thread>
#include<common.h>
#include<SingleInstance.h>

using namespace test::common;

namespace TestLspDiagnostic {
    bool LspDiagnosticTest(TestParam param)
    {
        SingleInstance *p = SingleInstance::GetInstance();
        std::string testFile = p->messagePath + "/" + param.testFile;

        std::string rootUri;
        bool isMultiModule = false;

        if (CreateMsg(p->pathIn, testFile, rootUri, isMultiModule) != true) {
            return false;
        }
        if (IsMacroExpandTest(rootUri)) {
            return true;
        }
        if (CreateBuildScript(p->pathBuildScript, testFile)) {
            BuildDynamicBinary(p->pathBuildScript);
        }
        /* Wait until the task is complete. The join blocking mode is not used. */
        StartLspServer();

        /* Check the test case result. */
        nlohmann::json expLines = ReadExpectedResult(param.baseFile);
        ChangeMessageUrlForBaseFile(testFile, expLines, rootUri, isMultiModule);
        nlohmann::json result = ReadFileById(p->pathOut, param.id);

        /* if case is diff show info */
        std::string reason = "none";
        bool showErr = CheckDiagnosticResult(expLines, result, reason);
        if (!showErr) {
            std::cout << "the false reason is : " << reason << std::endl;
            ShowDiff(expLines, result, param, p->messagePath);
        }
        return showErr;
    }

    class DiagnosticTest : public testing::TestWithParam<struct TestParam> {
    protected:
        void SetUp()
        {
            SetUpConfig("diagnostic");
        }
    };

    INSTANTIATE_TEST_SUITE_P(Diagnostic, DiagnosticTest, testing::ValuesIn(GetTestCaseList("diagnostic")));

    TEST_P(DiagnosticTest, DiagnosticCase)
    {
        TestParam param = GetParam();
        ASSERT_TRUE(LspDiagnosticTest(param));
    }
}