interface I4 {
    mut func f1():Unit
    func f2():Unit
}
struct R4 <: I4 {
    public mut func f1():Unit {}
    public func f2():Unit {}
}
class C4 <: I4 {
    public func f1():Unit {}
    public func f2():Unit {}
}
// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.
