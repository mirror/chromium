// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/DeclaredStylePropertyMap.h"

#include "core/css/StyleRule.h"

namespace blink {

DeclaredStylePropertyMap::DeclaredStylePropertyMap(StyleRule* owner_rule)
    : StylePropertyMap(), owner_rule_(owner_rule) {}

const CSSValue* DeclaredStylePropertyMap::GetProperty(
    CSSPropertyID property_id) {
  return owner_rule_->Properties().GetPropertyCSSValue(property_id);
}

const CSSValue* DeclaredStylePropertyMap::GetCustomProperty(
    AtomicString property_name) {
  return owner_rule_->Properties().GetPropertyCSSValue(property_name);
}

void DeclaredStylePropertyMap::SetProperty(CSSPropertyID property_id,
                                           const CSSValue* value) {
  owner_rule_->MutableProperties().SetProperty(property_id, *value);
}

void DeclaredStylePropertyMap::RemoveProperty(CSSPropertyID property_id) {
  owner_rule_->MutableProperties().RemoveProperty(property_id);
}

void DeclaredStylePropertyMap::ForEachProperty(
    const IterationCallback& callback) {
  const CSSPropertyValueSet& declared_style_set = owner_rule_->Properties();
  for (unsigned i = 0; i < declared_style_set.PropertyCount(); i++) {
    const auto& property = declared_style_set.PropertyAt(i);
    callback(property.Id(), property.Value());
  }
}

}  // namespace blink
