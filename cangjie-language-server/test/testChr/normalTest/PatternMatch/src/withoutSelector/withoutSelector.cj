// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package default.withoutSelector
func testWithoutSelector() {
    let score = 80
    var result: String = match {
    case score < 60 => "fail"
    case score < 70 => "pass"
    case score < 90 => "good" // matched
    case _ => "excellent"
    }
}
