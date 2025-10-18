// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

interface I {
    func test(a: Int64): Int64 {
        return 0
    }
}

open class F {}

extend F <: I {}

func main2() {
    var bb = F().
}
