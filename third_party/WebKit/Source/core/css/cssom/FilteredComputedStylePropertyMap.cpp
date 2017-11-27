// Copyright 2016 the Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/FilteredComputedStylePropertyMap.h"

#include "core/css/CSSCustomPropertyDeclaration.h"

namespace blink {

FilteredComputedStylePropertyMap::FilteredComputedStylePropertyMap(
    Node* node,
    const Vector<CSSPropertyID>& native_properties,
    const Vector<AtomicString>& custom_properties)
    : ComputedStylePropertyMap(node) {
  for (const auto& native_property : native_properties) {
    native_properties_.insert(native_property);
  }

  for (const auto& custom_property : custom_properties) {
    custom_properties_.insert(custom_property);
  }
}

const CSSValue* FilteredComputedStylePropertyMap::GetProperty(
    CSSPropertyID property_id) {
  if (!native_properties_.Contains(property_id))
    return nullptr;

  return ComputedStylePropertyMap::GetProperty(property_id);
}

const CSSValue* FilteredComputedStylePropertyMap::GetCustomProperty(
    AtomicString property_name) {
  if (!custom_properties_.Contains(AtomicString(property_name)))
    return nullptr;

  return ComputedStylePropertyMap::GetCustomProperty(property_name);
}

void FilteredComputedStylePropertyMap::ForEachProperty(
    const IterationCallback& callback) {
  ComputedStylePropertyMap::ForEachProperty([&](CSSPropertyID property_id,
                                                const CSSValue& value) {
    DCHECK(isValidCSSPropertyID(property_id));
    if (property_id == CSSPropertyVariable) {
      const auto& custom_declaration = ToCSSCustomPropertyDeclaration(value);
      if (custom_properties_.Contains(custom_declaration.GetName()))
        callback(property_id, value);
    } else if (native_properties_.Contains(property_id)) {
      callback(property_id, value);
    }
  });
}

}  // namespace blink
