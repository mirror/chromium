// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AtRuleDescriptorValueSet_h
#define AtRuleDescriptorValueSet_h

#include "platform/wtf/text/StringBuilder.h"

namespace blink {

class AtRuleDescriptorValueSet
    : public GarbageCollectedFinalized<AtRuleDescriptorValueSet> {
 public:
  static AtRuleDescriptorValueSet* Create(
      const HeapVector<blink::CSSPropertyValue, 256> properties) {
    return nullptr;
  }

  String AsText() const { return String(); }
  bool HasFailedOrCanceledSubresources() const { return false; }
  const CSSValue* GetPropertyCSSValue(AtRuleDescriptorID) const {
    return nullptr;
  }

  AtRuleDescriptorValueSet* MutableCopy() const { return nullptr; }
  bool IsMutable() { return true; }

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(values_);
    for (unsigned i = 0; i < values_.size(); i++) {
      visitor->Trace(values_[i].second);
    }
  }

 private:
  HeapVector<std::pair<AtRuleDescriptorID, Member<CSSValue>>> values_;
};

}  // namespace blink

#endif  // AtRuleDescriptorValueSet_h
