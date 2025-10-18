// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

class C13 {
    static prop ii: Int64 {
        get() {
            0
        }
    }
}

extend C13 {
    static prop i: Int64 {
        get() {
            0
        }
    }
}
let aa = C13.i
let aa2 = C13.ii

