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

#include "core/timing/PerformanceEntry.h"
#include "core/timing/PerformanceMarkOptions.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class CORE_EXPORT PerformanceMark final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PerformanceMark* Create(const String& name,
                                 double start_time,
                                 const ScriptValue& detail) {
    return new PerformanceMark(name, start_time, detail);
  }

  ScriptValue detail(ScriptState* script_state) const {
    v8::Isolate* isolate = script_state->GetIsolate();
    if (detail_.IsEmpty()) {
      printf("detail is empty.\n");
      return ScriptValue(script_state, v8::Null(isolate));
    }
    printf("detail is not empty.\n");
    // Todo: do we need to return a clone of |detail_| if the world is
    // different?
    return ScriptValue(script_state, detail_.NewLocal(isolate));
  }

  virtual void Trace(blink::Visitor* visitor) {
    PerformanceEntry::Trace(visitor);
  }

 private:
  PerformanceMark(const String& name,
                  double start_time,
                  const ScriptValue& detail)
      : PerformanceEntry(name, "mark", start_time, start_time), detail_(this) {
    if (detail.IsEmpty() || detail.IsNull() || detail.IsUndefined()) {
      printf("detail is nonexistant.\n");
      return;
    }
    printf("detail exists.\n");
    detail_.Set(detail.GetIsolate(), detail.V8Value());
  }

  ~PerformanceMark() override {}
  // Todo: is the TraceWrapperV8Reference necessary?
  TraceWrapperV8Reference<v8::Value> detail_;
};

}  // namespace blink

#endif  // PerformanceMark_h
