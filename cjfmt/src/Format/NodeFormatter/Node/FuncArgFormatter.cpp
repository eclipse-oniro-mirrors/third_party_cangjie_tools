// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "Format/NodeFormatter/Node/FuncArgFormatter.h"
#include "Format/ASTToFormatSource.h"
#include "cangjie/AST/Node.h"

namespace Cangjie::Format {
using namespace Cangjie::AST;

void FuncArgFormatter::ASTToDoc(Doc& doc, Ptr<Cangjie::AST::Node> node, int level, FuncOptions& funcOptions)
{
    auto funcArg = As<ASTKind::FUNC_ARG>(node);
    AddFuncArg(doc, *funcArg, level, funcOptions);
}
void FuncArgFormatter::AddFuncArg(Doc& doc, const Cangjie::AST::FuncArg& funcArg, int level, FuncOptions& funcOptions)
{
    doc.type = DocType::FUNC_ARG;
    doc.indent = level;

    Doc group(DocType::GROUP, level, "");
    if (funcArg.withInout) {
        group.members.emplace_back(DocType::STRING, level, "inout ");
    }
    if (!funcArg.name.GetRawText().empty()) {
        group.members.emplace_back(DocType::STRING, level, funcArg.name.GetRawText() + ": ");
    }
    group.members.emplace_back(astToFormatSource.ASTToDoc(funcArg.expr.get(), level, funcOptions));
    doc.members.emplace_back(group);
}
} // namespace Cangjie::Format