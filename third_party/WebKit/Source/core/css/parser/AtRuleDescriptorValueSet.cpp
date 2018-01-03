// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/AtRuleDescriptorValueSet.h"

#include "core/css/parser/AtRuleDescriptorParser.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserMode.h"

namespace blink {

AtRuleDescriptorValueSet* AtRuleDescriptorValueSet::Create(
    const HeapVector<blink::CSSPropertyValue, 256>& properties) {
  AtRuleDescriptorValueSet* set = new AtRuleDescriptorValueSet();
  for (const auto& property : properties) {
    set->values_.Set(CSSPropertyIDAsAtRuleDescriptor(property.Id()),
                     property.Value());
  }
  return set;
}

String AtRuleDescriptorValueSet::AsText() const {
  return "";
}

bool AtRuleDescriptorValueSet::HasFailedOrCanceledSubresources() const {
  for (const auto& value : values_.Values()) {
    if (value->HasFailedOrCanceledSubresources()) {
      return true;
    }
  }
  return false;
}

const CSSValue* AtRuleDescriptorValueSet::GetPropertyCSSValue(
    AtRuleDescriptorID id) const {
  const auto& iterator = values_.find(id);
  if (iterator == values_.end())
    return nullptr;

  return iterator->value;
}

AtRuleDescriptorValueSet::SetResult AtRuleDescriptorValueSet::SetProperty(
    AtRuleDescriptorID id, const String& value, SecureContextMode mode) {
  if (value.IsEmpty()) {
    if (values_.Contains(id))
      RemoveProperty(id);
    return SetResult{true /* did_parse */, true /* did_change */};
  }

  CSSParserContext* context = CSSParserContext::Create(kHTMLStandardMode, mode);
  const CSSValue* parsed_value = AtRuleDescriptorParser::ParseAtRule(
      id, value, *context);
  if (parsed_value) {
    values_.Set(id, parsed_value);
    return SetResult{true /* did_parse */, true /* did_change */};
  }
  return SetResult{false /* did_parse */, false /* did_change */};
}

AtRuleDescriptorValueSet* AtRuleDescriptorValueSet::MutableCopy() const {
  AtRuleDescriptorValueSet* set = new AtRuleDescriptorValueSet();
  for (const auto& entry : values_) {
    set->values_.Set(entry.key, entry.value);
  }
  return set;
}

void AtRuleDescriptorValueSet::ParseDeclarationList(
    const String& declaration, SecureContextMode mode) {
  // TODO(crbug.com/752745): Refactor CSSParserImpl to avoid
  // using CSSPropertyValueSet here.
  MutableCSSPropertyValueSet* property_value_set =
      MutableCSSPropertyValueSet::Create(kCSSViewportRuleMode);
  property_value_set->ParseDeclarationList(
      declaration, mode, nullptr /* context_style_sheet */);
  for (
}

void AtRuleDescriptorValueSet::Trace(blink::Visitor* visitor) {
  visitor->Trace(values_);
  for (const auto& value : values_.Values()) {
    visitor->Trace(value);
  }
}

void AtRuleDescriptorValueSet::RemoveProperty(AtRuleDescriptorID id) {
  values_.erase(id);
}

}  // namespace blink
