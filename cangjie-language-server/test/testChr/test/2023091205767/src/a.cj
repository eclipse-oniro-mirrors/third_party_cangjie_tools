// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

public open class A2{
    static public func ff_i() {}
}

class B2 <: A2{
    redef static public func ff_i() {}
}

func test2(){
    var a1 = B2.ff_i
}
