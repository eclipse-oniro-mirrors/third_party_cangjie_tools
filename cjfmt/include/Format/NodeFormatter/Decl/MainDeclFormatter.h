// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef CJFMT_MAINDECLFORMATTER_H
#define CJFMT_MAINDECLFORMATTER_H
#include "DeclFormatter.h"

namespace Cangjie::Format {
class MainDeclFormatter : public DeclFormatter {
public:
    explicit MainDeclFormatter(ASTToFormatSource& astToFormatSource, FormattingOptions& options)
        : DeclFormatter(astToFormatSource, options){};

    void ASTToDoc(
        Doc& doc, Ptr<Cangjie::AST::Node> node, int level, FuncOptions&) override;

private:
    void AddMainDecl(Doc& doc, const Cangjie::AST::MainDecl& mainDecl, int level);
};
} // namespace Cangjie::Format
#endif // CJFMT_MAINDECLFORMATTER_H
