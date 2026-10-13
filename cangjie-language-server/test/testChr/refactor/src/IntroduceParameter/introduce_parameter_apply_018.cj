// Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package ohos_app_cangjie_entry.IntroduceParameter

class CompoundAssignOwner {
    var offset: Int64 = 3

    func shift(base: Int64): Unit {
        this.offset <<= 2
    }
}

func callCompoundAssignOwner(): Unit {
    let owner = CompoundAssignOwner()
    owner.shift(10)
}
