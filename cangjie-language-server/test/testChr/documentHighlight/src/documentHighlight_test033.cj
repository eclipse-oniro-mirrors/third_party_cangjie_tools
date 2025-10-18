// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

import testpkg.testiner.inPkg

class testInpkg <: inPkg{
    var aa:int32 = 0
    init() {
        aa = inPkg.aclass
    }

    func foo():int32{
        return 1
    }
}

func pkg_test033(): int32 {
    var aaa = inPkg.getSum()
    return 1
}
