// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

open class C {
    let tryexprrr = 1
}
open class D <: C {}
class E <: D {}
func tt(): E {
    let y = E()
    try {
        y.
    } catch (e: Exception) {
        y
    } finally {
        y
    }
}
