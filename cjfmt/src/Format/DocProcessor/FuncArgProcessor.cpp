// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "Format/DocProcessor/FuncArgProcessor.h"

using namespace Cangjie::Format;

void FuncArgProcessor::DocToString(
    std::string& formatted, int& pos, std::pair<Doc, Mode>& current, std::vector<std::pair<Doc, Mode>>& leftCmd)
{
    for (auto it = current.first.members.rbegin(); it != current.first.members.rend(); ++it) {
        leftCmd.emplace_back(*it, current.second);
    }
}