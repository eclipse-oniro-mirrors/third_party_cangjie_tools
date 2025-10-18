// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

struct Test43{
    let width: int32 = 2
    let length1: int32 = 5

    func area1(): int32 {
        return width * length
    }
}

func semanticHighlight_test043():unit {
    var value:Test43 = Test43()
    var sum = value.area()
    var a = value.length
}
