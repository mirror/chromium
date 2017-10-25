// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/callback_interface.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off

#include "V8TestCallbackInterface.h"

#include "bindings/core/v8/GeneratedCodeHelper.h"
#include "bindings/core/v8/IDLTypes.h"
#include "bindings/core/v8/NativeValueTraitsImpl.h"
#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8TestInterfaceEmpty.h"
#include "core/dom/ExecutionContext.h"
#include "platform/wtf/Assertions.h"
#include "platform/wtf/GetPtr.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

void V8TestCallbackInterface::voidMethod() {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethod",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "voidMethod"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethod",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> *argv = nullptr;

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      0,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

v8::Maybe<bool> V8TestCallbackInterface::booleanMethod() {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "booleanMethod",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return v8::Nothing<bool>();
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "booleanMethod"))
        .ToLocal(&value)) {
      return v8::Nothing<bool>();
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "booleanMethod",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return v8::Nothing<bool>();
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> *argv = nullptr;

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      0,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return v8::Nothing<bool>();
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  {
    ExceptionState exception_state(GetIsolate(),
                                   ExceptionState::kExecutionContext,
                                   "TestCallbackInterface",
                                   "booleanMethod");
    auto native_result =
        NativeValueTraits<IDLBoolean>::NativeValue(
            GetIsolate(), call_result, exception_state);
    return exception_state.HadException() ?
        v8::Nothing<bool>() :
        v8::Just<bool>(native_result);
  }
}

void V8TestCallbackInterface::voidMethodBooleanArg(bool boolArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodBooleanArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "voidMethodBooleanArg"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodBooleanArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> boolArgHandle = v8::Boolean::New(GetIsolate(), boolArg);
  v8::Local<v8::Value> argv[] = { boolArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      1,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

void V8TestCallbackInterface::voidMethodSequenceArg(const HeapVector<Member<TestInterfaceEmpty>>& sequenceArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodSequenceArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "voidMethodSequenceArg"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodSequenceArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> sequenceArgHandle = ToV8(sequenceArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { sequenceArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      1,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

void V8TestCallbackInterface::voidMethodFloatArg(float floatArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodFloatArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "voidMethodFloatArg"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodFloatArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> floatArgHandle = v8::Number::New(GetIsolate(), floatArg);
  v8::Local<v8::Value> argv[] = { floatArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      1,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

void V8TestCallbackInterface::voidMethodTestInterfaceEmptyArg(TestInterfaceEmpty* testInterfaceEmptyArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodTestInterfaceEmptyArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "voidMethodTestInterfaceEmptyArg"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodTestInterfaceEmptyArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      1,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

void V8TestCallbackInterface::voidMethodTestInterfaceEmptyStringArg(TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "voidMethodTestInterfaceEmptyStringArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "voidMethodTestInterfaceEmptyStringArg"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "voidMethodTestInterfaceEmptyStringArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> stringArgHandle = V8String(GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle, stringArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      2,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

void V8TestCallbackInterface::callbackWithThisValueVoidMethodStringArg(ScriptValue thisValue, const String& stringArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "callbackWithThisValueVoidMethodStringArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "callbackWithThisValueVoidMethodStringArg"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "callbackWithThisValueVoidMethodStringArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> stringArgHandle = V8String(GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { stringArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      1,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

void V8TestCallbackInterface::customVoidMethodTestInterfaceEmptyArg(TestInterfaceEmpty* testInterfaceEmptyArg) {
  // This function implements "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation

  ScriptWrappable* callback_this_value = nullptr;

  if (!IsCallbackInterfaceRunnable(CallbackRelevantContext(),
                                   IncumbentContext())) {
    V8ThrowException::ThrowError(
        GetIsolate(),
        ExceptionMessages::FailedToExecute(
            "customVoidMethodTestInterfaceEmptyArg",
            "TestCallbackInterface",
            "The provided callback is no longer runnable."));
    return;
  }

  // step 7. Prepare to run script with relevant settings.
  ScriptState::Scope callback_relevant_context_scope(CallbackRelevantContext());
  // step 8. Prepare to run a callback with stored settings.
  v8::Context::BackupIncumbentScope backup_incumbent_scope(
      IncumbentContext()->GetContext());

  // As a nature of callback interfaces, there may not be author script
  // surrounding this function's invocation, i.e. no try-catch block to catch an
  // exception, and it's likely that an exception will be reported to top level,
  // e.g. DevTools console.  So this function throws an exception without storing
  // it into an ExceptionState argument and set the verbose flag to true.
  v8::TryCatch tryCatch(GetIsolate());
  tryCatch.SetVerbose(true);

  v8::Local<v8::Function> function;
  if (IsCallbackObjectCallable()) {
    // step 9.1. If value's interface is a single operation callback interface
    //   and !IsCallable(O) is true, then set X to O.
    function = CallbackObject().As<v8::Function>();
  } else {
    // step 9.2.1. Let getResult be Get(O, opName).
    // step 9.2.2. If getResult is an abrupt completion, set completion to
    //   getResult and jump to the step labeled return.
    v8::Local<v8::Value> value;
    if (!CallbackObject()->Get(CallbackRelevantContext()->GetContext(),
                               V8String(GetIsolate(), "customVoidMethodTestInterfaceEmptyArg"))
        .ToLocal(&value)) {
      return;
    }
    // step 10. If !IsCallable(X) is false, then set completion to a new
    //   Completion{[[Type]]: throw, [[Value]]: a newly created TypeError
    //   object, [[Target]]: empty}, and jump to the step labeled return.
    if (!value->IsFunction()) {
      V8ThrowException::ThrowTypeError(
          GetIsolate(),
          ExceptionMessages::FailedToExecute(
              "customVoidMethodTestInterfaceEmptyArg",
              "TestCallbackInterface",
              "The provided callback is not callable."));
      return;
    }
  }

  v8::Local<v8::Value> this_arg;
  if (!IsCallbackObjectCallable()) {
    // step 11. If value's interface is not a single operation callback
    //   interface, or if !IsCallable(O) is false, set thisArg to O (overriding
    //   the provided value).
    this_arg = CallbackObject();
  } else if (!callback_this_value) {
    // step 2. If thisArg was not given, let thisArg be undefined.
    this_arg = v8::Undefined(GetIsolate());
  } else {
    this_arg = ToV8(callback_this_value, CallbackRelevantContext());
  }

  // step 12. Let esArgs be the result of converting args to an ECMAScript
  //   arguments list. If this throws an exception, set completion to the
  //   completion value representing the thrown exception and jump to the step
  //   labeled return.
  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle };

  // step 13. Let callResult be Call(X, thisArg, esArgs).
  v8::MaybeLocal<v8::Value> maybe_call_result = V8ScriptRunner::CallFunction(
      function,
      ExecutionContext::From(CallbackRelevantContext()),
      this_arg,
      1,
      argv,
      GetIsolate());

  // step 14. If callResult is an abrupt completion, set completion to
  //   callResult and jump to the step labeled return.
  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return;
  }
  // step 15. Set completion to the result of converting callResult.[[Value]] to
  //   an IDL value of the same type as the operation's return type.
  return;
}

}  // namespace blink
