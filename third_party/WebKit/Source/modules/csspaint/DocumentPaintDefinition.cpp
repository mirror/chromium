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

DocumentPaintDefinition::~DocumentPaintDefinition() {}

}  // namespace blink
