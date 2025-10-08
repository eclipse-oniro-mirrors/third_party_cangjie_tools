// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

package my_module

import ohos.ark_interop.*
import ohos.ark_interop_helper.*
import ohos.base.*
import std.collection.{ HashMap }

public type ErrorCode = Float64


/***********ENUM***********/

/* enum EventType {
	}*/
public enum EventType <: ToString & Equatable < EventType > {
    
    func get(): Int32 {
        match(this) {
        }
    }
    
    static func parse(val: Int32): EventType {
        match(val) {
            case _ => throw IllegalArgumentException("unknown value ${val}")
        }
    }
    
    static func tryParse(val: ?Int32): ?EventType {
        match(val) {
            case Some(v) => parse(v)
            case None => None
        }
    }
    
    public func toString(): String {
        match(this) {
        }
    }
    
    public override operator func ==(that: EventType): Bool {
        match((this, that)) {
            case _ => false
        }
    }
    
    public override operator func !=(that: EventType): Bool {
        !(this == that)
    }
}


/***********OBJECT***********/

/*interface TestListener {
    onStart?: () => void;
    onDestroy?: () => void;
    onError?: (code: ErrorCode, msg: String) => void;
    onTouch?: () => void;
    onEvent?: (e: EventType) => void;
    }*/

public open class TestListener {
    
    protected TestListener(public var onStart!: Option<() -> Unit> = None,
    public var onDestroy!: Option<() -> Unit> = None,
    public var onError!: Option<(code: ErrorCode, msg: String) -> Unit> = None,
    public var onTouch!: Option<() -> Unit> = None,
    public var onEvent!: Option<(e: EventType) -> Unit> = None) {}
    
    
    public open func toJSValue(context: JSContext): JSValue {
        let obj = context.object()
        if(let Some(v) <- onStart) {
            obj["onStart"] = context.function({ ctx, _ =>
                v()
                ctx.undefined().toJSValue()
            }).toJSValue()
        }
        if(let Some(v) <- onDestroy) {
            obj["onDestroy"] = context.function({ ctx, _ =>
                v()
                ctx.undefined().toJSValue()
            }).toJSValue()
        }
        if(let Some(v) <- onError) {
            obj["onError"] = context.function({ ctx, info =>
                let p0 = ErrorCode.fromJSValue(ctx, info[0])
                let p1 = String.fromJSValue(ctx, info[1])
                v(p0, p1)
                ctx.undefined().toJSValue()
            }).toJSValue()
        }
        if(let Some(v) <- onTouch) {
            obj["onTouch"] = context.function({ ctx, _ =>
                v()
                ctx.undefined().toJSValue()
            }).toJSValue()
        }
        if(let Some(v) <- onEvent) {
            obj["onEvent"] = context.function({ ctx, info =>
                let p0 = EventType.parse(Int32.fromJSValue(ctx, info[0]))
                v(p0)
                ctx.undefined().toJSValue()
            }).toJSValue()
        }
        obj.toJSValue()
    }
    
    public static func fromJSValue(context: JSContext, input: JSValue): TestListener {
        let obj = input.asObject()
        TestListener(
        onStart: if(obj["onStart"].isUndefined()) {
            None
        } else {
            { =>
                checkThreadAndCall < Unit >(context, { _ =>
                    obj["onStart"].asFunction().call()
                })
            }
        },
        onDestroy: if(obj["onDestroy"].isUndefined()) {
            None
        } else {
            { =>
                checkThreadAndCall < Unit >(context, { _ =>
                    obj["onDestroy"].asFunction().call()
                })
            }
        },
        onError: if(obj["onError"].isUndefined()) {
            None
        } else {
            { code: ErrorCode, msg: String =>
                checkThreadAndCall < Unit >(context, { ctx =>
                    let arg0 = code.toJSValue(ctx)
                    let arg1 = msg.toJSValue(ctx)
                    obj["onError"].asFunction().call([arg0, arg1])
                })
            }
        },
        onTouch: if(obj["onTouch"].isUndefined()) {
            None
        } else {
            { =>
                checkThreadAndCall < Unit >(context, { _ =>
                    obj["onTouch"].asFunction().call()
                })
            }
        },
        onEvent: if(obj["onEvent"].isUndefined()) {
            None
        } else {
            { e: EventType =>
                checkThreadAndCall < Unit >(context, { ctx =>
                    let arg0 = e.get().toJSValue(ctx)
                    obj["onEvent"].asFunction().call([arg0])
                })
            }
        }
        )
    }
    
}

/*interface MyListener {
    on(key: string, param: boolean, cb: (r: Record<string, string>) => void): void
    }*/

public open class MyListener {
    let callbackManager = CallbackManager < String, JSValue >()
    
    
    protected MyListener(public var arkts_object: JSObject) {}
    
    
    /**
	 * @brief on(key: string, param: boolean, cb: (r: Record<string, string>) => void): void
	 */
    public func on(key: String, param: Bool, cb: Callback1Argument<JSValue/* FIXME: `Record` */>): Unit {
        let key = key.toString()
        if(callbackManager.findCallbackObject(key, cb).isSome()) {
            return
        }
        let jsCallback = checkThreadAndCall < JSValue >(getMainContext()) {
            __ctx => __ctx.function {
                __ctx: JSContext, info: JSCallInfo =>
                let arg0 = Record<JSValue/* FIXME: `stringstring` */>.fromJSValue(__ctx, info[0])
                cb.invoke(arg0)
                __ctx.undefined().toJSValue()
            }.toJSValue()
        }
        callbackManager.put(key,(cb, jsCallback))
        jsObjApiCall < Unit >( arkts_object, "on", { __ctx =>[key.toJSValue(__ctx), param.toJSValue(__ctx), jsCallback] })
    }
    
    public open func toJSValue(context: JSContext): JSValue {
        arkts_object.toJSValue()
    }
    
    static func fromJSValue(context: JSContext, input: JSValue): MyListener {
        MyListener(input.asObject())
    }
}

