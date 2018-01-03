// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AtRuleCSSStyleDeclaration_h
#define AtRuleCSSStyleDeclaration_h

#include "core/css/CSSFontFaceRule.h"
#include "core/css/CSSRule.h"
#include "core/css/CSSStyleDeclaration.h"
#include "core/css/parser/AtRuleDescriptorValueSet.h"

namespace blink {

class CORE_EXPORT AtRuleCSSStyleDeclaration : public CSSStyleDeclaration {
 public:
  static AtRuleCSSStyleDeclaration* Create(AtRuleDescriptorValueSet* set,
                                           CSSFontFaceRule* parent_rule) {
    return new AtRuleCSSStyleDeclaration(set, parent_rule);
  }

  CSSRule* parentRule() const final { return parent_rule_.Get(); }

  String cssText() const final;
  void setCSSText(const ExecutionContext*,
                  const String&,
                  ExceptionState&) final;

  unsigned length() const final { return 0; }

  String item(unsigned index) const final;

  String getPropertyValue(const String& property_name) final;
  String getPropertyPriority(const String& property_name) final;
  String GetPropertyShorthand(const String& property_name) final;
  bool IsPropertyImplicit(const String& property_name) final;

  void setProperty(const ExecutionContext*,
                   const String& property_name,
                   const String& value,
                   const String& priority,
                   ExceptionState&) final;
  String removeProperty(const String& property_name, ExceptionState&) final;

  void Reattach(const AtRuleDescriptorValueSet&);

  // To support bindings and editing. These are not used for @rules. See
  // CSSStyleDeclaration.h for details.
  const CSSValue* GetPropertyCSSValueInternal(CSSPropertyID) final {
    NOTREACHED();
    return nullptr;
  }
  const CSSValue* GetPropertyCSSValueInternal(
      AtomicString custom_property_name) final {
    NOTREACHED();
    return nullptr;
  }
  String GetPropertyValueInternal(CSSPropertyID) final {
    NOTREACHED();
    return "";
  }
  void SetPropertyInternal(CSSPropertyID,
                           const String& property_value,
                           const String& value,
                           bool important,
                           SecureContextMode,
                           ExceptionState&) {
    NOTREACHED();
  }
  bool CssPropertyMatches(CSSPropertyID, const CSSValue*) const final {
    return false;
  }

  void Trace(blink::Visitor*) override;

 private:
  AtRuleCSSStyleDeclaration(AtRuleDescriptorValueSet* set, CSSRule* parent_rule)
      : descriptor_value_set_(set), parent_rule_(parent_rule) {}

  Member<AtRuleDescriptorValueSet> descriptor_value_set_;
  TraceWrapperMember<CSSRule> parent_rule_;
};

}  // namespace blink

#endif  // AtRuleCSSStyleDeclaration_h
