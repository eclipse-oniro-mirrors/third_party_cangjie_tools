// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "Format/NodeFormatter/Node/FuncParamFormatter.h"
#include "Format/ASTToFormatSource.h"
#include "cangjie/AST/Node.h"

namespace Cangjie::Format {
using namespace Cangjie::AST;

void FuncParamFormatter::ASTToDoc(Doc& doc, Ptr<Cangjie::AST::Node> node, int level, FuncOptions& funcOptions)
{
    auto param = As<ASTKind::FUNC_PARAM>(node);
    AddFuncParam(doc, *param, level, funcOptions);
}
void FuncParamFormatter::AddFuncParam(
    Doc& doc, const Cangjie::AST::FuncParam& funcParam, int level, FuncOptions funcOptions)
{
    doc.type = DocType::CONCAT;
    doc.indent = level;
    doc.members.emplace_back(DocType::SOFTLINE, level, "");
    if (!funcParam.annotations.empty()) {
        astToFormatSource.AddAnnotations(doc, funcParam.annotations, level, false);
    }
    astToFormatSource.AddModifier(doc, funcParam.modifiers, level);
    if (funcParam.isMemberParam) {
        doc.members.emplace_back(DocType::STRING, level, funcParam.isVar ? "var " : "let ");
    }
    if (funcOptions.patternOrEnum) {
        doc.members.emplace_back(astToFormatSource.ASTToDoc(funcParam.type.get(), level));
    } else {
        std::string id = funcParam.identifier.GetRawText() + (funcParam.isNamedParam ? "!" : "");
        if (funcParam.type) {
            doc.members.emplace_back(DocType::STRING, level, id + ": ");
            doc.members.emplace_back(astToFormatSource.ASTToDoc(funcParam.type.get(), level));
        } else {
            doc.members.emplace_back(DocType::STRING, level, id);
        }
    }
    if (funcParam.assignment) {
        doc.members.emplace_back(DocType::STRING, level, " = ");
        doc.members.emplace_back(astToFormatSource.ASTToDoc(funcParam.assignment.get(), level));
    }
}
} // namespace Cangjie::Format