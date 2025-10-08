// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef CJFMT_MEMBERACCESSPROCESSOR_H
#define CJFMT_MEMBERACCESSPROCESSOR_H
#include "Format/DocProcessor/DocProcessor.h"

namespace Cangjie::Format {
class MemberAccessProcessor : public DocProcessor {
public:
    explicit MemberAccessProcessor(ASTToFormatSource& astToFormatSource, FormattingOptions& options)
        : DocProcessor(astToFormatSource, options){};
    void DocToString(std::string& formatted, int& pos, std::pair<Doc, Mode>& current,
        std::vector<std::pair<Doc, Mode>>& leftCmd) override;

private:
    void NonMethodChianProcessor(Doc& member, int& pos, std::string& formatted);
};
} // namespace Cangjie::Format
#endif // CJFMT_MEMBERACCESSPROCESSOR_H
