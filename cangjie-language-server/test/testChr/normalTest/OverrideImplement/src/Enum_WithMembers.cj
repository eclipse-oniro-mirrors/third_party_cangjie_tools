// Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package UnimplementQuickFix

interface EnumWithMemberInterface {
    func doEnum(): Unit
}

enum MyEnum <: EnumWithMemberInterface {
    | VAL1 | VAL2 | VAL3

    public func existingMethod(): Unit {}
}
