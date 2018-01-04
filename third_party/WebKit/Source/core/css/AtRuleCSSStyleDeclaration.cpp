// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/AtRuleCSSStyleDeclaration.h"

#include "core/css/CSSStyleSheet.h"
#include "core/css/StyleAttributeMutationScope.h"

#include <iostream>

namespace blink {

String AtRuleCSSStyleDeclaration::cssText() const {
  StringBuilder result;
  bool is_first = true;
  // Skip 0 (AtRuleDescriptorID::Invalid).
  for (unsigned i = 1; i < numAtRuleDescriptors; i++) {
    AtRuleDescriptorID id = static_cast<AtRuleDescriptorID>(i);
    const CSSValue* value = descriptor_value_set_->GetPropertyCSSValue(id);
    if (!value)
      continue;

    if (!is_first) {
      result.Append(' ');
    } else {
      is_first = false;
    }
    result.Append(GetDescriptorName(id));
    result.Append(": ");
    result.Append(value->CssText());
    result.Append(";");
  }
  return result.ToString();
}

void AtRuleCSSStyleDeclaration::setCSSText(const ExecutionContext* execution_context,
                                           const String& new_text,
                                           ExceptionState& exception_state) {
  StyleAttributeMutationScope mutation_scope(this);
  if (parent_rule_->parentStyleSheet())
    parent_rule_->parentStyleSheet()->WillMutateRules();

  // A null execution_context may be passed in by the inspector.
  const SecureContextMode mode = execution_context
                                     ? execution_context->GetSecureContextMode()
                                     : SecureContextMode::kInsecureContext;
  descriptor_value_set_->ParseDeclarationList(new_text, mode);

  // Style sheet mutation needs to be signaled even if the change failed.
  // willMutateRules/didMutateRules must pair.
  if (parent_rule_->parentStyleSheet())
    parent_rule_->parentStyleSheet()->DidMutateRules();

  mutation_scope.EnqueueMutationRecord();
}

String AtRuleCSSStyleDeclaration::item(unsigned index) const {
  if (index >= numAtRuleDescriptors) {
    return "";
  }

  return GetDescriptorName(static_cast<AtRuleDescriptorID>(index));
}

String AtRuleCSSStyleDeclaration::getPropertyValue(
    const String& property_name) {
  AtRuleDescriptorID id = AsAtRuleDescriptorID(property_name);
  if (id == AtRuleDescriptorID::Invalid)
    return "";

  return descriptor_value_set_->GetPropertyCSSValue(id)->CssText();
}

String AtRuleCSSStyleDeclaration::getPropertyPriority(
    const String& property_name) {
  return "";
}

String AtRuleCSSStyleDeclaration::GetPropertyShorthand(
    const String& property_name) {
  return "";
}

bool AtRuleCSSStyleDeclaration::IsPropertyImplicit(
    const String& property_name) {
  return false;
}

void AtRuleCSSStyleDeclaration::setProperty(const ExecutionContext* context,
                                            const String& property_name,
                                            const String& value, const String& priority, ExceptionState& exception_state) {
  AtRuleDescriptorID id = AsAtRuleDescriptorID(property_name);
  if (id == AtRuleDescriptorID::Invalid)
    return;

  StyleAttributeMutationScope mutation_scope(this);
  bool did_change = false;

  descriptor_value_set_->SetProperty(id, value, context->GetSecureContextMode());

  if (parent_rule_->parentStyleSheet())
    parent_rule_->parentStyleSheet()->DidMutateRules();
  if (!did_change)
    return;

  mutation_scope.EnqueueMutationRecord();
}

String AtRuleCSSStyleDeclaration::removeProperty(
    const String& property_name,
    ExceptionState& exception_state) {
  AtRuleDescriptorID id = AsAtRuleDescriptorID(property_name);
  if (id == AtRuleDescriptorID::Invalid)
    return "";
  const CSSValue* value = descriptor_value_set_->GetPropertyCSSValue(id);
  descriptor_value_set_->RemoveProperty(id);
  return value ? value->CssText() : "";
}

void AtRuleCSSStyleDeclaration::Reattach(const AtRuleDescriptorValueSet& set) {}

void AtRuleCSSStyleDeclaration::Trace(blink::Visitor* visitor) {
  visitor->Trace(descriptor_value_set_);
  CSSStyleDeclaration::Trace(visitor);
}

}  // namespace blink
