/*
 * Copyright (C) 2017 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bindings/core/v8/V8LazyErrorHandler.h"

#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8ErrorEvent.h"
#include "bindings/core/v8/V8ScriptRunner.h"
#include "core/dom/ExecutionContext.h"
#include "platform/bindings/V8PrivateProperty.h"

namespace blink {

V8LazyErrorHandler::V8LazyErrorHandler(
    const AtomicString& function_name,
    const Vector<AtomicString>& event_parameter_names,
    const String& code,
    const String source_url,
    const TextPosition& position,
    Node* node,
    v8::Isolate* isolate)
    : V8LazyEventListener(isolate,
                          function_name,
                          event_parameter_names,
                          code,
                          source_url,
                          position,
                          node) {}

v8::Local<v8::Value> V8LazyErrorHandler::CallListenerFunction(
    ScriptState* script_state,
    v8::Local<v8::Value> js_event,
    Event* event) {
  DCHECK(!js_event.IsEmpty());
  if (!event->HasInterface(EventNames::ErrorEvent)) {
    return V8LazyEventListener::CallListenerFunction(script_state, js_event,
                                                     event);
  }

  ErrorEvent* error_event = static_cast<ErrorEvent*>(event);
  if (error_event->World() && error_event->World() != &World())
    return v8::Null(GetIsolate());

  v8::Local<v8::Context> context = script_state->GetContext();
  ExecutionContext* execution_context = ToExecutionContext(context);
  v8::Local<v8::Object> listener = GetListenerObject(execution_context);
  if (listener.IsEmpty() || !listener->IsFunction())
    return v8::Null(GetIsolate());

  v8::Local<v8::Function> call_function =
      v8::Local<v8::Function>::Cast(listener);
  v8::Local<v8::Object> this_value = context->Global();

  v8::Local<v8::Object> event_object;
  if (!js_event->ToObject(context).ToLocal(&event_object))
    return v8::Null(GetIsolate());
  auto private_error = V8PrivateProperty::GetErrorEventError(GetIsolate());
  v8::Local<v8::Value> error = private_error.GetOrUndefined(event_object);
  if (error->IsUndefined()) {
    ScriptValue error_value = error_event->error(script_state);
    error = error_value.IsEmpty() ? v8::Local<v8::Value>(v8::Null(GetIsolate()))
                                  : error_value.V8Value();
  }

  v8::Local<v8::Value> parameters[5] = {
      V8String(GetIsolate(), error_event->message()),
      V8String(GetIsolate(), error_event->filename()),
      v8::Integer::New(GetIsolate(), error_event->lineno()),
      v8::Integer::New(GetIsolate(), error_event->colno()), error};
  v8::TryCatch try_catch(GetIsolate());
  try_catch.SetVerbose(true);

  v8::MaybeLocal<v8::Value> result = V8ScriptRunner::CallFunction(
      call_function, execution_context, this_value,
      WTF_ARRAY_LENGTH(parameters), parameters, GetIsolate());
  v8::Local<v8::Value> return_value;
  if (!result.ToLocal(&return_value))
    return v8::Null(GetIsolate());

  return return_value;
}

bool V8LazyErrorHandler::ShouldPreventDefault(
    v8::Local<v8::Value> return_value) {
  return return_value->IsBoolean() && return_value.As<v8::Boolean>()->Value();
}

}  // namespace blink
