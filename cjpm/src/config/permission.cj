// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package cjpm.config

@When[os == "Linux" || os == "macOS"]
import std.fs.*

@When[os == "Linux" || os == "macOS"]
// change file permissions to read、write and not executable
public func modifyFilePermission(filePath: String): Bool {
    try {
        let fileInfo: FileInfo = FileInfo(filePath)
        if (!fileInfo.setReadable(true) || !fileInfo.setWritable(true) || !fileInfo.setExecutable(false)) {
            eprintln("Error: failed to change permission of file '${filePath}'")
            return false
        }
        return true
    } catch (e: Exception) {
        eprintln(e.message)
        eprintln("Error: failed to change permission of file '${filePath}'")
        return false
    }
}

@When[os == "Windows"]
public func modifyFilePermission(_: String): Bool {
    return true
}
