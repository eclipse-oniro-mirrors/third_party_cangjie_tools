// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef CJHEAD_COMPILER_INSTANCE_H
#define CJHEAD_COMPILER_INSTANCE_H
#include "cangjie/Frontend/CompilerInstance.h"
using namespace Cangjie;
using namespace Cangjie::AST;

namespace CJHead {
class CJHeadCompilerInstance : public CompilerInstance {
public:
    CJHeadCompilerInstance(CompilerInvocation &invocation, DiagnosticEngine &diag) : CompilerInstance(invocation, diag)
    {}

    bool Compile(CompileStage stage) override;
};
}  // namespace CJHead

#endif