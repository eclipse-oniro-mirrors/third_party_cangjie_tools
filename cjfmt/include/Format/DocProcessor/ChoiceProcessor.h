// Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef CJFMT_CHOICEPROCESSOR_H
#define CJFMT_CHOICEPROCESSOR_H
#include "Format/DocProcessor/DocProcessor.h"

namespace Cangjie::Format {
class ChoiceProcessor : public DocProcessor {
public:
    using DocProcessor::DocProcessor;
    void DocToString(std::string&, int&, std::pair<Doc, Mode>&,
        std::vector<std::pair<Doc, Mode>>&) override;
};
}
#endif
