// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "Format/NodeFormatter/Expr/WildcardExprFormatter.h"
#include "Format/ASTToFormatSource.h"
#include "cangjie/AST/Node.h"

namespace Cangjie::Format {
using namespace Cangjie::AST;

void WildcardExprFormatter::ASTToDoc(Doc& doc, Ptr<Cangjie::AST::Node>, int level, FuncOptions&)
{
    AddWildcardExpr(doc, level);
}

void WildcardExprFormatter::AddWildcardExpr(Doc& doc, int level)
{
    doc.type = DocType::CONCAT;
    doc.indent = level;
    doc.members.emplace_back(DocType::STRING, level, "_");
}
} // namespace Cangjie::Format