// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef CJFMT_ASSIGNEXPRFORMATTER_H
#define CJFMT_ASSIGNEXPRFORMATTER_H
#include "ExprFormatter.h"
namespace Cangjie::Format {
class AssignExprFormatter : public ExprFormatter {
public:
    explicit AssignExprFormatter(ASTToFormatSource& astToFormatSource, FormattingOptions& options)
        : ExprFormatter(astToFormatSource, options){};

    void ASTToDoc(Doc& doc, Ptr<Cangjie::AST::Node> node, int level, FuncOptions&) override;

private:
    void AddAssignExpr(Doc& doc, const Cangjie::AST::AssignExpr& assignExpr, int level);
};
} // namespace Cangjie::Format
#endif // CJFMT_ASSIGNEXPRFORMATTER_H
