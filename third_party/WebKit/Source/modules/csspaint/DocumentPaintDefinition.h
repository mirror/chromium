// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentPaintDefinition_h
#define DocumentPaintDefinition_h

#include "core/css/CSSSyntaxDescriptor.h"
#include "core/css/cssom/CSSStyleValue.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"

namespace blink {

class DocumentPaintDefinition final
    : public GarbageCollectedFinalized<DocumentPaintDefinition>,
      public TraceWrapperBase {
 public:
  static DocumentPaintDefinition* Create(const Vector<CSSPropertyID>&,
                                         const Vector<AtomicString>&,
                                         const Vector<CSSSyntaxDescriptor>&,
                                         bool has_alpha);
  virtual ~DocumentPaintDefinition();

  const Vector<CSSPropertyID>& NativeInvalidationProperties() const {
    return native_invalidation_properties_;
  }
  const Vector<AtomicString>& CustomInvalidationProperties() const {
    return custom_invalidation_properties_;
  }
  const Vector<CSSSyntaxDescriptor>& InputArgumentTypes() const {
    return input_argument_types_;
  }
  bool HasAlpha() const { return has_alpha_; }

  bool Equals(const DocumentPaintDefinition&) const;

  DEFINE_INLINE_TRACE(){};
  DECLARE_TRACE_WRAPPERS(){};

 private:
  DocumentPaintDefinition(const Vector<CSSPropertyID>&,
                          const Vector<AtomicString>&,
                          const Vector<CSSSyntaxDescriptor>&,
                          bool has_alpha);
  Vector<CSSPropertyID> native_invalidation_properties_;
  Vector<AtomicString> custom_invalidation_properties_;
  Vector<CSSSyntaxDescriptor> input_argument_types_;
  bool has_alpha_;
};

}  // namespace blink

#endif  // DocumentPaintDefinition_h
