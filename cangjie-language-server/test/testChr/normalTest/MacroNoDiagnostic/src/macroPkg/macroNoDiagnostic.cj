// Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

macro package default.macroPkg
import std.ast.*

public macro testDef(input: Tokens): Tokens {
    return input + quote(class AA {func foo(a: String) {}})
}
