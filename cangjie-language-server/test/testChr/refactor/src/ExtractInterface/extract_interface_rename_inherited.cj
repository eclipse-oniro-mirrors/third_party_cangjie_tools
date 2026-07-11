// Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

public interface Named {
    func name(): String
}

public class User <: Named {
    public func name(): String {
        return "user"
    }
    public func enabled(): Bool {
        return true
    }
}
