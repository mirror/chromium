// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentPaintDefinition_h
#define DocumentPaintDefinition_h

#include "core/css/CSSSyntaxDescriptor.h"
#include "core/css/cssom/CSSStyleValue.h"
#include "modules/csspaint/CSSPaintDefinition.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"

namespace blink {

class DocumentPaintDefinition final
    : public GarbageCollectedFinalized<DocumentPaintDefinition>,
      public TraceWrapperBase {
 public:
  DocumentPaintDefinition(CSSPaintDefinition*);
  virtual ~DocumentPaintDefinition();

  const Vector<CSSPropertyID>& NativeInvalidationProperties() const {
    return paint_definition_->NativeInvalidationProperties();
  }
  const Vector<AtomicString>& CustomInvalidationProperties() const {
    return paint_definition_->CustomInvalidationProperties();
  }
  const Vector<CSSSyntaxDescriptor>& InputArgumentTypes() const {
    return paint_definition_->InputArgumentTypes();
  }
  bool HasAlpha() const { return paint_definition_->HasAlpha(); }

  bool Matches(const CSSPaintDefinition&) const;

  DECLARE_VIRTUAL_TRACE();
  DECLARE_TRACE_WRAPPERS(){};

 private:
  Member<CSSPaintDefinition> paint_definition_;
};

}  // namespace blink

#endif  // DocumentPaintDefinition_h
