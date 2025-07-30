// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.
type ErrorCode = number;

enum EventType {}

interface TestListener {
    "onStart"?: () => void;
    "onDestroy"?: () => void;
    onError?: (code: ErrorCode, msg: string) => void;
    onTouch?: () => void;
    onEvent?: (e: EventType) => void;
}

interface MyListener {
    on(key: string, param: boolean, cb: (r: Record<string, string>) => void);
}
