// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PerformanceElementTiming_h
#define PerformanceElementTiming_h

#include "core/CoreExport.h"
#include "core/timing/PerformanceEntry.h"

namespace blink {

class ScriptState;
class V8ObjectBuilder;

class CORE_EXPORT PerformanceElementTiming final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum class ElementTypeEnum { kText };

  PerformanceElementTiming(ElementTypeEnum,
                           const String& name,
                           double start_time);
  ~PerformanceElementTiming() override;

  String elementType() const;

 protected:
  void BuildJSONValue(ScriptState*, V8ObjectBuilder&) const override;

 private:
  ElementTypeEnum element_type_;
};
}  // namespace blink

#endif  // PerformanceElementTiming_h
