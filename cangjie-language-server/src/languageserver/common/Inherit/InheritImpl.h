// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_INHERITIMPL_H
#define LSPSERVER_INHERITIMPL_H

#include <queue>
#include "cangjie/AST/Node.h"

namespace ark {
    std::vector<Ptr<Cangjie::AST::InheritableDecl>> GetTopClassDecl(
        Cangjie::AST::InheritableDecl &decl, bool isLib = false);
}
#endif // LSPSERVER_INHERITIMPL_H
