// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

public class A {
    public let a: Int64 = 1
}

public let array: Array<A> = [A(), A(), A()]


public func foo() {
    for (i in array) {
        if (i.a != 1) {
            continue
        }
        i.
    }
}
