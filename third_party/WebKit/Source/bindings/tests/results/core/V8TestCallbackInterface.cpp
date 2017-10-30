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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> *argv = 0;

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          0,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

v8::Maybe<bool> V8TestCallbackInterface::booleanMethod() {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> *argv = 0;

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          0,
          argv,
          isolate);

  v8::Local<v8::Value> call_result;
  if (!maybe_call_result.ToLocal(&call_result)) {
    return v8::Nothing<bool>();
  }
  ExceptionState exception_state(GetIsolate(),
                                 ExceptionState::kExecutionContext,
                                 "TestCallbackInterface",
                                 "booleanMethod");
  bool native_result = NativeValueTraits<IDLBoolean>::NativeValue(
      isolate, call_result, exception_state);
  return exception_state.HadException() ?
      v8::Nothing<bool>() : v8::Just<bool>(native_result);
}

void V8TestCallbackInterface::voidMethodBooleanArg(bool boolArg) {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> boolArgHandle = v8::Boolean::New(GetIsolate(), boolArg);
  v8::Local<v8::Value> argv[] = { boolArgHandle };

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          1,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

void V8TestCallbackInterface::voidMethodSequenceArg(const HeapVector<Member<TestInterfaceEmpty>>& sequenceArg) {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> sequenceArgHandle = ToV8(sequenceArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { sequenceArgHandle };

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          1,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

void V8TestCallbackInterface::voidMethodFloatArg(float floatArg) {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> floatArgHandle = v8::Number::New(GetIsolate(), floatArg);
  v8::Local<v8::Value> argv[] = { floatArgHandle };

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          1,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

void V8TestCallbackInterface::voidMethodTestInterfaceEmptyArg(TestInterfaceEmpty* testInterfaceEmptyArg) {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle };

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          1,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

void V8TestCallbackInterface::voidMethodTestInterfaceEmptyStringArg(TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> stringArgHandle = V8String(GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle, stringArgHandle };

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          2,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

void V8TestCallbackInterface::callbackWithThisValueVoidMethodStringArg(ScriptValue thisValue, const String& stringArg) {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> stringArgHandle = V8String(GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { stringArgHandle };

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          1,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

void V8TestCallbackInterface::customVoidMethodTestInterfaceEmptyArg(TestInterfaceEmpty* testInterfaceEmptyArg) {
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

  ScriptState::Scope scope(CallbackRelevantContext());

  v8::Local<v8::Object> argument_creation_context =
      CallbackRelevantContext()->GetContext()->Global();
  ALLOW_UNUSED_LOCAL(argument_creation_context);
  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, argument_creation_context, GetIsolate());
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle };

  v8::Isolate* isolate = GetIsolate();

  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);

  v8::MaybeLocal<v8::Value> maybe_call_result =
      V8ScriptRunner::CallFunction(
          CallbackObject().As<v8::Function>(),
          ExecutionContext::From(CallbackRelevantContext()),
          v8::Undefined(isolate),
          1,
          argv,
          isolate);

  ALLOW_UNUSED_LOCAL(maybe_call_result);
}

}  // namespace blink
