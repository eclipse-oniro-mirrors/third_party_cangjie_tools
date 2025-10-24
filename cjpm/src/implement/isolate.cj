// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package cjpm.implement

import cjpm.config.*

func supportLto(): Bool {
    if (crossCompileTarget.isEmpty()) {
        return targetConfigName.contains("linux") || targetConfigName.contains("ohos") ||
        targetConfigName.contains("android")
    }
    return crossCompileTarget.contains("linux") || crossCompileTarget.contains("ohos") ||
        crossCompileTarget.contains("android")
}

func isSupportCombine(): Bool {
    let compileTarget = if (crossCompileTarget.isEmpty()) {
            targetConfigName
        } else {
            crossCompileTarget
        }
    let isTargetLinux = compileTarget.contains("linux")
    let isTargetAndroid = compileTarget.contains("android")
    let isTargetWindows = compileTarget.contains("windows")
    return ((isTargetLinux && !isTargetAndroid) || isTargetWindows)
}