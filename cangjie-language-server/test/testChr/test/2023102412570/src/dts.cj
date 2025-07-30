@Annotation
public class C19 {
    const C19(a:String,b:Int64) {}
    const init() {}
}

@C19
interface I2{
    @C19
    func hc():Unit
}
// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.
