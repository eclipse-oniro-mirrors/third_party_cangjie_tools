// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

import testpkg.testpkgFunc
import testpkg01.testpkgFunc

func pkg_test046(): int32 {
    let a = testpkg.testpkgFunc()
    let d = testpkg01.testpkgFunc()
    return 0
}
