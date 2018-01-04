// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AtRuleDescriptorValueSet_h
#define AtRuleDescriptorValueSet_h

#include "core/css/CSSPropertyValue.h"
#include "core/css/parser/AtRuleDescriptors.h"
#include "core/css/parser/CSSParserMode.h"
#include "core/dom/ExecutionContext.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

class CORE_EXPORT AtRuleDescriptorValueSet
    : public GarbageCollectedFinalized<AtRuleDescriptorValueSet> {
 public:
  struct SetResult {
    bool did_parse;
    bool did_change;
  };

  static AtRuleDescriptorValueSet* Create(
      const HeapVector<blink::CSSPropertyValue, 256>& properties, CSSParserMode);

  String AsText() const;
  bool HasFailedOrCanceledSubresources() const;
  const CSSValue* GetPropertyCSSValue(AtRuleDescriptorID) const;
  struct SetResult SetProperty(
      AtRuleDescriptorID, const String& value, SecureContextMode);
  void SetProperty(AtRuleDescriptorID, const CSSValue&);

  AtRuleDescriptorValueSet* MutableCopy() const;
  bool IsMutable() { return true; }

  void RemoveProperty(AtRuleDescriptorID);

  void ParseDeclarationList(const String& declaration, SecureContextMode mode);

  void Trace(blink::Visitor* visitor);

 private:
  AtRuleDescriptorValueSet(CSSParserMode mode) : mode_(mode) {}

  CSSParserMode mode_;
  HeapHashMap<AtRuleDescriptorID, Member<const CSSValue>> values_;
};

}  // namespace blink

#endif  // AtRuleDescriptorValueSet_h
