// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AtRuleCSSStyleDeclaration_h
#define AtRuleCSSStyleDeclaration_h

#include "core/css/CSSStyleDeclaration.h"
#include "core/css/parser/AtRuleDescriptorValueSet.h"

namespace blink {

class CORE_EXPORT AtRuleCSSStyleDeclaration : public CSSStyleDeclaration {
 public:
  static AtRuleCSSStyleDeclaration* Create(const AtRuleDescriptorValueSet& set,
                                           const CSSFontFaceRule&) {
    return new AtRuleCSSStyleDeclaration();
  }

  CSSRule* parentRule() const final { return nullptr; }

  String cssText() const final { return ""; }
  void setCSSText(const ExecutionContext*,
                  const String&,
                  ExceptionState&) final {}

  unsigned length() const final { return 0; }

  String item(unsigned index) const final { return ""; }

  String getPropertyValue(const String& property_name) final { return ""; }
  String getPropertyPriority(const String& property_name) final { return ""; }
  String GetPropertyShorthand(const String& property_name) final { return ""; }
  bool IsPropertyImplicit(const String& property_name) final { return false; }

  void setProperty(const ExecutionContext*,
                   const String& property_name,
                   const String& value,
                   const String& priority,
                   ExceptionState&) final {}
  String removeProperty(const String& property_name, ExceptionState&) final {
    return "";
  }

  void Reattach(const AtRuleDescriptorValueSet&) {}

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
};

}  // namespace blink

#endif  // AtRuleCSSStyleDeclaration_h
