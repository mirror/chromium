/*
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PerformanceMark_h
#define PerformanceMark_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "bindings/core/v8/serialization/SerializedScriptValueFactory.h"
#include "core/timing/PerformanceEntry.h"
#include "platform/bindings/DOMWrapperWorld.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ScriptWrappableVisitor;

class CORE_EXPORT PerformanceMark final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PerformanceMark* Create(ScriptState* script_state,
                                 const String& name,
                                 double start_time,
                                 const ScriptValue& detail) {
    return new PerformanceMark(script_state, name, start_time, detail);
  }

  ScriptValue detail(ScriptState* script_state) const {
    v8::Isolate* isolate = script_state->GetIsolate();
    if (detail_.IsEmpty()) {
      return ScriptValue(script_state, v8::Null(isolate));
    }
    // Return a serialized clone when the world is different.
    if (!world_ || world_->GetWorldId() != script_state->World().GetWorldId()) {
      v8::Local<v8::Value> value = detail_.NewLocal(isolate);
      scoped_refptr<SerializedScriptValue> serialized =
          SerializedScriptValue::SerializeAndSwallowExceptions(isolate, value);
      return ScriptValue(script_state, serialized->Deserialize(isolate));
    }
    return ScriptValue(script_state, detail_.NewLocal(isolate));
  }

  virtual void Trace(blink::Visitor* visitor) {
    PerformanceEntry::Trace(visitor);
  }

  virtual void TraceWrappers(const ScriptWrappableVisitor* visitor) const {
    visitor->TraceWrappers(detail_);
  }

 private:
  PerformanceMark(ScriptState* script_state,
                  const String& name,
                  double start_time,
                  const ScriptValue& detail)
      : PerformanceEntry(name, "mark", start_time, start_time), detail_(this) {
    world_ = world_ = WrapRefCounted(&script_state->World());
    if (detail.IsEmpty() || detail.IsNull() || detail.IsUndefined()) {
      return;
    }
    detail_.Set(detail.GetIsolate(), detail.V8Value());
  }

  ~PerformanceMark() override = default;

  scoped_refptr<DOMWrapperWorld> world_;
  TraceWrapperV8Reference<v8::Value> detail_;
};

}  // namespace blink

#endif  // PerformanceMark_h
