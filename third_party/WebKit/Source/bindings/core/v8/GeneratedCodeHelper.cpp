// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/core/v8/GeneratedCodeHelper.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8EventListenerHelper.h"
#include "bindings/core/v8/serialization/SerializedScriptValue.h"

#include <tuple>
#include <utility>

namespace blink {

void V8ConstructorAttributeGetter(
    v8::Local<v8::Name> property_name,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(
      info.GetIsolate(), "Blink_V8ConstructorAttributeGetter");
  v8::Local<v8::Value> data = info.Data();
  DCHECK(data->IsExternal());
  V8PerContextData* per_context_data =
      V8PerContextData::From(info.Holder()->CreationContext());
  if (!per_context_data)
    return;
  V8SetReturnValue(info, per_context_data->ConstructorForType(
                             WrapperTypeInfo::Unwrap(data)));
}

void V8EventHandlerGetter(const v8::FunctionCallbackInfo<v8::Value>& info,
                          V8EventHandlerGetterImpl impl) {
  ScriptWrappable* wrappable = ToScriptWrappable(info.Holder());
  std::pair<EventListener*, ExecutionContext*> result = impl(wrappable);
  v8::Local<v8::Value> listener_value;
  if (EventListener* listener = result.first) {
    listener_value = V8AbstractEventListener::Cast(listener)->GetListenerObject(
        result.second);
  }
  if (listener_value.IsEmpty())
    V8SetReturnValueNull(info);
  else
    V8SetReturnValue(info, listener_value);
}

void V8EventHandlerSetter(v8::Local<v8::Value> v8Value,
                          const v8::FunctionCallbackInfo<v8::Value>& info,
                          V8EventHandlerSetterImpl impl) {
  ScriptWrappable* wrappable = ToScriptWrappable(info.Holder());
  V8EventListener* listener = V8EventListenerHelper::GetEventListener(
      ScriptState::ForRelevantRealm(info), v8Value, true,
      kListenerFindOrCreate);
  impl(wrappable, listener);
}

v8::Local<v8::Value> V8Deserialize(v8::Isolate* isolate,
                                   PassRefPtr<SerializedScriptValue> value) {
  if (value)
    return value->Deserialize(isolate);
  return v8::Null(isolate);
}

}  // namespace blink
