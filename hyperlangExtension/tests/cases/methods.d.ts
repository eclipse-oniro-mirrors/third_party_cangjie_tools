// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.
enum EMNum {
    E1 = 111,
    E2 = 222,
    E3 = 333,
}

enum EMStr {
    EA = "aaa",
    EB = "bbb",
    EC = "ccc",
}

interface IM {
    f1(): void;
    f2(s: string): number;
    f3(r: Record<string, string>): EMNum
    f4(e: EMStr): (s: string) => void
    f5(cb: (obj: CM, n: number) => string): void
    f6(evt: EMNum, cb: () => Record<number, Uint8Array>): boolean
    f7(p: bigint, cb: (p: bigint) => void): bigint
}

type Callback<T> = (arg: T) => void;

declare class CM {
    // 静态成员
    static sa: string;
    static f00(x: (obj: IM, cb: (msg: EMStr) => void) => void): void;

    static f01(x: (obj: IM, cb: Callback<EMStr>) => void): void; // this is the same as f00, but depends on type inference
    ma: string;
    mb: EMStr;
    mc: (s: string, n: EMNum) => void;

    f1(): void;
    f2(s: string): number;
    f3(r: Record<string, string>): EMNum
    f4(e: EMStr): (s: string) => void
    f5(cb: (s: string, n: number) => string): void
    f6(evt: EMNum, cb: () => Record<number, Uint8Array>): boolean
    f7(p: bigint, cb: (p: bigint) => void): bigint
    f8(result: string | Promise<string>): boolean
}

declare class TE {
  /**
  * @since 1.0.0
  */
  constructor(
    param1?:Array<UInt8>,
    param2?:Array<string>
    callback:AsyncCallback<string>,
  )
  f1(): void;
  f1(result: string | Promise<string>): Promise<string>;
}

declare function test(): Promise<string>

declare function onEnumCb(p1: number | boolean, p2: Record<string, number> | Uint8Array | string): string

// 构造函数
declare class Greeter {
    constructor(greeting: string);
}

  declare class Person {
    // 私有属性
    private age: number;

  }

  declare class AnimalProtect {
    // 受保护属性
    protected name: string;

    // 受保护方法
    protected makeSound(): void;
}

declare class Car {
    // 只读属性
    readonly brand: string;
    name: string
}

// 泛型成员
declare class Box<T> {
    // 属性
    value: T;
    // 方法
    getValue(): T;
  }

// 抽象类
declare abstract class Shape {
    // 抽象方法
    abstract getArea(): number;
}

// 类实现接口
interface Drivable {
    start(): void;
    stop(): void;
  }

  declare class Car1 implements Drivable {
    start(): void;
    stop(): void;
}

// 继承类
declare class Animal {
    name: string;
    constructor(name: string);
    move(distance: number): void;
  }

  // Dog.d.ts
  declare class Dog extends Animal {
    bark(): void;
}

// 重载方法
declare class Calculator {
    // 方法重载
    add(x: number, y: number): number;
    add(x: string, y: string): string;

    // 实现
    add(x: any, y: any): any;
  }

  // 带装饰器的类
declare function logClass(target: any): void;

@logClass
declare class MyClass {
  name: string;
  constructor(name: string);
}

// 泛型
declare function getFavoriteNumber<T>(t: T): T;

export declare function test(): Promise<void>;
export declare function test1(name:string): Promise<void>;

declare function testUnion1(t: string | undefined ): undefined | string;

declare function testUnion2(t: null | T, s: A | B): null | string;

declare function testUnion3(t: number[] | null): null | number[];

declare function testUnion4(t: Record<string, Uint8Array>| null): void;

export declare class UMConfig {
  appKey?: string;
  channel?: string;
  enableLog?: boolean;
}

export declare class RecordTest {
  recordTest: Record<string, string>
  recordTest1: Record<string, string | number>
  recordTest2: Record<string, string> | null
  recordTest3: Record<string, string | number> | null
}

export declare function RecordAndOptionalTest(eventID: string, params?: Record<string, string | number>): void;

export declare function RecordTest(eventID: string, params: Record<string, string | number>): void;

export declare function RecordAndOptionalTest1(eventID: string, params?: Record<string, string | number>, params1?: Record<string, string | number>): void;

export declare function RecordTest1(eventID: string, params: Record<string, string | number>, params1: Record<string, string | number>): void;

export declare function RecordAndOptionalTest2(eventID: string, params?: Record<string, string | number>, params1?: Record<string, string | number>): void;

export declare function RecordTest2(eventID: string, params: Record<string, number>, params1: Record<string, number>): void;

export declare function RecordAndOptionalTest3(eventID: string, params: Record<string, string | number> | null, params1: Record<string, string | number>|undefined): void;
