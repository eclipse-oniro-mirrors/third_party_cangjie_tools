// Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package ParenCompletion

import std.net.*

class Server <: Resource {
    public init() {}
    public func isClosed(): Bool { true }
    public func close(): Unit {}
    public prop localAddress: SocketAddress {
        get() { IPSocketAddress("127.0.0.1", 0) }
    }
}

var SERVER_PORT: UInt16 = 0

func run() {
    try (serverSocket = Server()) {
        SERVER_PORT = (serverSocket.localAddress as IPSocketAddress)?.port ?? 0
    }
}

func readAddress(serverSocket: Server): SocketAddress {
    return ((serverSocket.localAddress))
}
