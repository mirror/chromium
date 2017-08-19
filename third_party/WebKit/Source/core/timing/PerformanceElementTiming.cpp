// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/PerformanceElementTiming.h"

#include "bindings/core/v8/V8ObjectBuilder.h"
#include "platform/bindings/ScriptState.h"

namespace blink {

PerformanceElementTiming::PerformanceElementTiming(ElementTypeEnum element_type,
                                                   const String& name,
                                                   double start_time)
    : PerformanceEntry(name, "elementtiming", start_time, start_time),
      element_type_(element_type) {}

PerformanceElementTiming::~PerformanceElementTiming() {}

String PerformanceElementTiming::elementType() const {
  switch (element_type_) {
    case ElementTypeEnum::kText:
      return "text";
  }
}

void PerformanceElementTiming::BuildJSONValue(ScriptState* script_state,
                                              V8ObjectBuilder& builder) const {
  PerformanceEntry::BuildJSONValue(script_state, builder);
  builder.AddString("elementType", elementType());
}

}  // namespace blink
