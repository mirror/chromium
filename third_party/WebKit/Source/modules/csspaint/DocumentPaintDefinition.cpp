// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/DocumentPaintDefinition.h"

namespace blink {

DocumentPaintDefinition* DocumentPaintDefinition::Create(
    const Vector<CSSPropertyID>& native_invalidation_properties,
    const Vector<AtomicString>& custom_invalidation_properties,
    const Vector<CSSSyntaxDescriptor>& input_argument_types,
    bool has_alpha) {
  return new DocumentPaintDefinition(native_invalidation_properties,
                                     custom_invalidation_properties,
                                     input_argument_types, has_alpha);
}

DocumentPaintDefinition::DocumentPaintDefinition(
    const Vector<CSSPropertyID>& native_invalidation_properties,
    const Vector<AtomicString>& custom_invalidation_properties,
    const Vector<CSSSyntaxDescriptor>& input_argument_types,
    bool has_alpha)
    : has_alpha_(has_alpha) {
  native_invalidation_properties_ = native_invalidation_properties;
  custom_invalidation_properties_ = custom_invalidation_properties;
  input_argument_types_ = input_argument_types;
}

DocumentPaintDefinition::~DocumentPaintDefinition() {}

bool DocumentPaintDefinition::Equals(
    const DocumentPaintDefinition& other) const {
  if (has_alpha_ != other.HasAlpha())
    return false;
  if (native_invalidation_properties_ != other.NativeInvalidationProperties() ||
      custom_invalidation_properties_ != other.CustomInvalidationProperties() ||
      input_argument_types_ != other.InputArgumentTypes())
    return false;
  return true;
}

}  // namespace blink
