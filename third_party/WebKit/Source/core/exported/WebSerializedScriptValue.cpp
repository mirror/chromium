/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "public/web/WebSerializedScriptValue.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "bindings/core/v8/serialization/SerializedScriptValueFactory.h"
#include "public/platform/WebString.h"

namespace blink {

// static
scoped_refptr<WebSerializedScriptValue> WebSerializedScriptValue::FromString(
    const WebString& s) {
  return SerializedScriptValue::Create(s);
}

// static
scoped_refptr<WebSerializedScriptValue> WebSerializedScriptValue::Serialize(
    v8::Isolate* isolate,
    v8::Local<v8::Value> value) {
  DummyExceptionStateForTesting exception_state;
  scoped_refptr<WebSerializedScriptValue> serialized_value =
      SerializedScriptValue::Serialize(
          isolate, value, SerializedScriptValue::SerializeOptions(),
          exception_state);
  if (exception_state.HadException())
    return CreateInvalid();
  return serialized_value;
}

// static
scoped_refptr<WebSerializedScriptValue>
WebSerializedScriptValue::CreateInvalid() {
  return SerializedScriptValue::Create();
}

WebSerializedScriptValue::WebSerializedScriptValue() {}
WebSerializedScriptValue::~WebSerializedScriptValue() {}

scoped_refptr<SerializedScriptValue>
WebSerializedScriptValue::AsSerializedScriptValue() {
  return static_cast<SerializedScriptValue*>(this);
}

}  // namespace blink
