// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package test

public open class Test {
	public init( a:Int32) {}
	public init(a:Int64) {}
	public init( a!:Bool) {}
	public init(c!:String = "cc") {}
	public Test(a:Int64,b!:Int64) {a+b}
	public init(a:Int64,b!:Int32,c!:Bool=true){}
    public var abc = 0
}
