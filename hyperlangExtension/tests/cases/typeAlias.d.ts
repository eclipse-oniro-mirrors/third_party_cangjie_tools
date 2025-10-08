// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.
type TA10 = number;
type TA20 = string;
type TA30 = boolean;
type TA40 = bigint;
type TA50 = object;
type TA60 = symbol;
type TA70 = void;
type TA80 = undefined;
type TA90 = any;
type TA100 = unknown;
type TA110 = never;

type TA200 = () => void;
type TA210<V> = (arg: V) => void;
type TA220<V, W extends string> = (arg: V) => W;
type TA230<V = object, W extends string = '123' | '345'> = (arg: V) => W; // type parameter default values are unsupported
type TA240 = (...a: number[]) => void;
type TA250 = (a?: number) => void;

type TA300 = Promise<string>;
type TA310 = Promise<string>[];

type TA400 = Pick<Promise<void>, 'then'>;
type TA410 = Omit<Promise<void>, ''>;
type TA420 = Omit<Promise<void>, ''>;

type TA500 = 123;
type TA510 = 'abc';
type TA520 = null;

type TA600 = [number, string, number];

type TA700 = Record<string, unknown> | null;
type TA710 = "aaa" | "bbb" | "ccc" | "ddd";
type TA720 = number | string;
type TA730 = Promise<string> | string;
type TA740 = "111" | TA710;

type TA810 = { x: number; y: string };
type TA820 = { [p: number]: string };
type TA830 = { (arg: number): string };

type TA900 = readonly number[];
type TA910 = keyof { x: number; y: string };

type TA1000 = typeof setTimeout;
type TA1010 = ReturnType<typeof setTimeout>;

type ARK1 = null | number | string | boolean | Uint8Array | Float32Array | bigint | Int8Array | Int16Array | Uint16Array | Uint32Array | Int32Array | BigInt64Array | BigUint64Array | Float64Array;

// 交叉类型别名
type User = {
    id: number;
    name: string;
} & { isActive: boolean };

// 泛型类型别名
type ApiResponse<T> = {
    data: T;
    status: number;
};

export type AnimationItem = {
    name: string;
    name1: string;
    play11(name?: string,name1?: string): void;
};