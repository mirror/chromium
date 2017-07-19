// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/DocumentPaintDefinition.h"

namespace blink {

DocumentPaintDefinition::DocumentPaintDefinition(CSSPaintDefinition* definition)
    : paint_definition_(definition) {
  DCHECK(definition);
}

DocumentPaintDefinition::~DocumentPaintDefinition() {}

bool DocumentPaintDefinition::Matches(const CSSPaintDefinition& other) const {
  if (HasAlpha() != other.HasAlpha() ||
      NativeInvalidationProperties() != other.NativeInvalidationProperties() ||
      CustomInvalidationProperties() != other.CustomInvalidationProperties() ||
      InputArgumentTypes() != other.InputArgumentTypes())
    return false;
  return true;
}

DEFINE_TRACE(DocumentPaintDefinition) {
  visitor->Trace(paint_definition_);
}

}  // namespace blink
