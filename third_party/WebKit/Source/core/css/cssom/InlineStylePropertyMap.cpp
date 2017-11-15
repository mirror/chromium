// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/InlineStylePropertyMap.h"

#include "bindings/core/v8/Iterable.h"
#include "core/CSSPropertyNames.h"
#include "core/css/CSSCustomIdentValue.h"
#include "core/css/CSSCustomPropertyDeclaration.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/CSSPropertyValueSet.h"
#include "core/css/CSSValueList.h"
#include "core/css/cssom/CSSUnsupportedStyleValue.h"
#include "core/css/cssom/StyleValueFactory.h"
#include "core/css/properties/CSSProperty.h"

namespace blink {

const CSSValue* InlineStylePropertyMap::GetProperty(CSSPropertyID property_id) {
  return owner_element_->EnsureMutableInlineStyle().GetPropertyCSSValue(
      property_id);
}

const CSSValue* InlineStylePropertyMap::GetCustomProperty(
    AtomicString property_name) {
  return owner_element_->EnsureMutableInlineStyle().GetPropertyCSSValue(
      property_name);
}

void InlineStylePropertyMap::SetProperty(CSSPropertyID property_id,
                                         const CSSValue* value) {
  owner_element_->SetInlineStyleProperty(property_id, value);
}

void InlineStylePropertyMap::RemoveProperty(CSSPropertyID property_id) {
  owner_element_->RemoveInlineStyleProperty(property_id);
}

void InlineStylePropertyMap::ForEachProperty(
    const IterationCallback& callback) {
  CSSPropertyValueSet& inline_style_set =
      owner_element_->EnsureMutableInlineStyle();
  for (unsigned i = 0; i < inline_style_set.PropertyCount(); i++) {
    const auto& property_reference = inline_style_set.PropertyAt(i);
    callback(property_reference.Id(), property_reference.Value());
  }
}

}  // namespace blink
