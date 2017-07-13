// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/DocumentPaintDefinition.h"

namespace blink {

DocumentPaintDefinition* DocumentPaintDefinition::Create(
    Vector<CSSPropertyID>& native_invalidation_properties,
    Vector<AtomicString>& custom_invalidation_properties,
    Vector<CSSSyntaxDescriptor>& input_argument_types,
    bool has_alpha) {
  return new DocumentPaintDefinition(native_invalidation_properties,
                                     custom_invalidation_properties,
                                     input_argument_types, has_alpha);
}

DocumentPaintDefinition::DocumentPaintDefinition(
    Vector<CSSPropertyID>& native_invalidation_properties,
    Vector<AtomicString>& custom_invalidation_properties,
    Vector<CSSSyntaxDescriptor>& input_argument_types,
    bool has_alpha)
    : has_alpha_(has_alpha) {
  native_invalidation_properties_.swap(native_invalidation_properties);
  custom_invalidation_properties_.swap(custom_invalidation_properties);
  input_argument_types_.swap(input_argument_types);
}

bool DocumentPaintDefinition::Equals(DocumentPaintDefinition* other) const {
  if (has_alpha_ != other->HasAlpha())
    return false;
  if ((native_invalidation_properties_.size() !=
       other->NativeInvalidationProperties().size()) ||
      (custom_invalidation_properties_.size() !=
       other->CustomInvalidationProperties().size()) ||
      (input_argument_types_.size() != other->InputArgumentTypes().size()))
    return false;
  for (size_t i = 0; i < native_invalidation_properties_.size(); i++) {
    if (native_invalidation_properties_[i] !=
        other->NativeInvalidationProperties()[i])
      return false;
  }
  for (size_t i = 0; i < custom_invalidation_properties_.size(); i++) {
    if (custom_invalidation_properties_[i] !=
        other->CustomInvalidationProperties()[i])
      return false;
  }
  for (size_t i = 0; i < input_argument_types_.size(); i++) {
    if (input_argument_types_[i] != other->InputArgumentTypes()[i])
      return false;
  }
  return true;
}

DocumentPaintDefinition::~DocumentPaintDefinition() {}

}  // namespace blink
