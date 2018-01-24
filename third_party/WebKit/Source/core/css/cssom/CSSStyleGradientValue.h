// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSStyleGradientValue_h
#define CSSStyleGradientValue_h

#include "base/macros.h"
#include "core/css/cssom/CSSStyleImageValue.h"

namespace blink {

namespace cssvalue {
class CSSGradientValue;
}
class Node;

// CSSStyleGradientValue is the Typed OM representation of gradients.
// There is no corresponding JavaScript class; it is simply an
// implementation of CSSStyleImageValue for <gradient> values.
class CORE_EXPORT CSSStyleGradientValue : public CSSStyleImageValue {
 public:
  static CSSStyleGradientValue* FromCSSValue(
      const cssvalue::CSSGradientValue& value,
      Node* node = nullptr) {
    return new CSSStyleGradientValue(value, node);
  }

  // CSSStyleImageValue
  WTF::Optional<IntSize> IntrinsicSize() const final { return WTF::nullopt; }

  // CanvasImageSource
  ResourceStatus Status() const final { return ResourceStatus::kCached; }
  scoped_refptr<Image> GetSourceImageForCanvas(SourceImageStatus*,
                                               AccelerationHint,
                                               const FloatSize&);
  // DO NOT SUBMIT: not sure what to return here.
  bool IsAccelerated() const final { return false; }

  // CSSStyleValue
  StyleValueType GetType() const override { return CSSStyleValue::kImageType; }
  const CSSValue* ToCSSValue() const final;

  virtual void Trace(blink::Visitor*);

 protected:
  CSSStyleGradientValue(const cssvalue::CSSGradientValue& value, Node* node)
      : value_(value), node_(node) {}

 private:
  Member<const cssvalue::CSSGradientValue> value_;
  WeakMember<Node> node_;
  DISALLOW_COPY_AND_ASSIGN(CSSStyleGradientValue);
};

}  // namespace blink

#endif  // CSSResourceValue_h
