// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

macro package BBB

from std import ast.*


public macro B1(input: Tokens): Tokens {
    return quote(public class C1{
        public var ab =1
        func gg1(x:Int64){}
    })
}
