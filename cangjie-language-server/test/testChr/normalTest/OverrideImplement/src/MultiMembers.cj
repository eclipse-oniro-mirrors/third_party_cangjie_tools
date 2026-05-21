// Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package UnimplementQuickFix

interface MultiInterface {
    func func1(): Unit
    func func2(a: Int32): Int32
    prop prop1: String
    mut prop prop2: Float64
}

class MultiClass <: MultiInterface {}
