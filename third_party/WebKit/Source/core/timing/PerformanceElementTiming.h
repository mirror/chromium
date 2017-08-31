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

// PerformanceElementTiming is a PerformanceEntry that contains display timing
// information for elements with an "elementtiming" attribute. Currently, only
// text elements are supported and the proposal is still a work in progress.
//
// More information can be found in the current proposal:
// https://docs.google.com/document/d/1sBM5lzDPws2mg1wRKiwM0TGFv9WqI6gEdF7vYhBYqUg/
class CORE_EXPORT PerformanceElementTiming final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum class ElementType { kText };

  PerformanceElementTiming(ElementType, const String& name, double start_time);
  ~PerformanceElementTiming() override;

  String elementType() const;

 protected:
  void BuildJSONValue(ScriptState*, V8ObjectBuilder&) const override;

 private:
  ElementType element_type_;
};
}  // namespace blink

#endif  // PerformanceElementTiming_h
