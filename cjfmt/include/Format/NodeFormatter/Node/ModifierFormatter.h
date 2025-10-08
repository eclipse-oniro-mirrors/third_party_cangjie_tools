// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef CJFMT_MODIFIERFORMATTER_H
#define CJFMT_MODIFIERFORMATTER_H
#include "Format/NodeFormatter/NodeFormatter.h"

namespace Cangjie::Format {
class ModifierFormatter : public NodeFormatter {
public:
    explicit ModifierFormatter(ASTToFormatSource& astToFormatSource, FormattingOptions& options)
        : NodeFormatter(astToFormatSource, options){};

    void ASTToDoc(Doc& doc, Ptr<Cangjie::AST::Node> node, int level, FuncOptions&) override;

private:
    void AddAnnotation(Doc& doc, const Cangjie::AST::Annotation& annotation, int level);
};
} // namespace Cangjie::Format
#endif // CJFMT_MODIFIERFORMATTER_H
