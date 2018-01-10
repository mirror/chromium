// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/AtRuleDescriptorValueSet.h"

#include "core/css/AtRuleDescriptorSerializer.h"
#include "core/css/CSSPropertyValueSet.h"
#include "core/css/parser/AtRuleDescriptorParser.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserMode.h"

namespace blink {

AtRuleDescriptorValueSet* AtRuleDescriptorValueSet::Create(
    const HeapVector<blink::CSSPropertyValue, 256>& properties,
    CSSParserMode mode,
    AtRuleType type) {
  AtRuleDescriptorValueSet* set = new AtRuleDescriptorValueSet(mode, type);
  for (const auto& property : properties) {
    set->values_.Set(CSSPropertyIDAsAtRuleDescriptor(property.Id()),
                     property.Value());
  }
  return set;
}

String AtRuleDescriptorValueSet::AsText() const {
  return AtRuleDescriptorSerializer::SerializeAtRuleDescriptors(*this);
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

void AtRuleDescriptorValueSet::SetProperty(AtRuleDescriptorID id,
                                           const String& value,
                                           SecureContextMode mode) {
  if (value.IsEmpty()) {
    if (values_.Contains(id))
      RemoveProperty(id);
    return;
  }

  CSSParserContext* context = CSSParserContext::Create(kHTMLStandardMode, mode);
  const CSSValue* parsed_value =
      AtRuleDescriptorParser::ParseAtRule(id, value, *context);
  if (parsed_value) {
    values_.Set(id, parsed_value);
  }
}

void AtRuleDescriptorValueSet::SetProperty(AtRuleDescriptorID id,
                                           const CSSValue& value) {
  values_.Set(id, value);
}

AtRuleDescriptorValueSet* AtRuleDescriptorValueSet::MutableCopy() const {
  AtRuleDescriptorValueSet* set = new AtRuleDescriptorValueSet(mode_, type_);
  for (const auto& entry : values_) {
    set->values_.Set(entry.key, entry.value);
  }
  return set;
}

void AtRuleDescriptorValueSet::ParseDeclarationList(const String& declaration,
                                                    SecureContextMode mode) {
  // TODO(crbug.com/752745): Refactor CSSParserImpl to avoid
  // using CSSPropertyValueSet here.
  MutableCSSPropertyValueSet* property_value_set =
      MutableCSSPropertyValueSet::Create(mode_);
  property_value_set->ParseDeclarationList(declaration, mode,
                                           nullptr /* context_style_sheet */);
  for (unsigned i = 0; i < property_value_set->PropertyCount(); i++) {
    CSSPropertyValueSet::PropertyReference reference =
        property_value_set->PropertyAt(i);
    values_.Set(CSSPropertyIDAsAtRuleDescriptor(reference.Id()),
                reference.Value());
  }
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
