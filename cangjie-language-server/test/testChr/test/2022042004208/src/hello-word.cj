// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

//  打印输出字符串
func callcangjie() {
    let hello:String = "你好"
    let cangjie:String = "仓颉"

    println("${hello}，${cangjie}！")
}

main() {
    return callcangjie()
}
