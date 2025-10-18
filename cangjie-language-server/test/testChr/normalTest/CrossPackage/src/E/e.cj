// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package default.E

import default.A.*

// enum 拓展接口场景  Scenarios6
enum EEEE {
    A | B | C
}

extend EEEE <: AInterface {
    public func funcAold(): Unit {
        return
    }
}

var bB = EEEE.A.funcAold()


