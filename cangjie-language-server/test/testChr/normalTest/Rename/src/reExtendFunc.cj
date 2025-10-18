// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

interface Sizeable {
    func size123(): Unit
}

class CC1{}
extend CC1 <: Sizeable {
    public func size123(): Unit {}
}

let c133: Sizeable = CC1()
let c233 = c133.size123()
