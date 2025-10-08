public class Data37 {
    var res1:int32 = 100
    static var res11:int32 = 1

    init(a: int32, b: int32) {
        this.res1 = a + b
    }
    func add22(a: int32, b: int32) {
        return a + b
    }
    static func add33(a: int32, b: int32) {
        return a + b
    }
}

func semanticHighlight_test037(): int64 {
    var a:Data37 = Data37(1, 2.0)
    var c = a.res1
    var d = a.add22(1,2)
    var e = Data37.res11
    var f = Data37.add33(1,2)
    return 1
}
// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.
