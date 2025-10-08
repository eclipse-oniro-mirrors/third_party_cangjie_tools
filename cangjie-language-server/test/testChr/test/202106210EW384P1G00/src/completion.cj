from std import sync.*

func test(){
    let mu:Mutex=Mutex()
    for(i in 0..100) {
        spawn { =>
            mu.lock()
            mu.newCondition()
            mu.isLocked()
        }
    }
    for(i in 0..100) {
        mu.
        mu.
        mu.
        mu.
        mu.
    }
    mu.newCondition()
}
// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.
