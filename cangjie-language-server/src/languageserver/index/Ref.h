// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_INDEX_REF_H
#define LSPSERVER_INDEX_REF_H

#include <cstdint>
#include "Symbol.h"
namespace ark {
namespace lsp {
enum class RefKind : uint8_t {
    UNKNOWN = 0,
    DEFINITION = 1,
    REFERENCE = 1 << 1,
    ALL = DEFINITION | REFERENCE
};

inline RefKind operator&(RefKind a, RefKind b)
{
    return static_cast<RefKind>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

struct Ref {
    SymbolLocation location;
    RefKind kind = RefKind::UNKNOWN;
    SymbolID container{};
    bool isCjoRef{false};
};

using RefSlab = std::map<SymbolID, std::vector<Ref>>;

} // namespace lsp
} // namespace ark
#endif // LSPSERVER_INDEX_REF_H