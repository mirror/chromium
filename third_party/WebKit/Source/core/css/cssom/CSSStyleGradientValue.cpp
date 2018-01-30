// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/CSSStyleGradientValue.h"

#include "core/css/CSSGradientValue.h"
#include "core/layout/LayoutObject.h"
#include "platform/graphics/Gradient.h"
#include "platform/graphics/GradientGeneratedImage.h"

namespace blink {

scoped_refptr<Image> CSSStyleGradientValue::GetSourceImageForCanvas(
    SourceImageStatus*,
    AccelerationHint,
    const FloatSize& default_object_size) {
  if (!node_)
    return nullptr;

  const LayoutObject* layout_object = node_->GetLayoutObject();
  if (!layout_object || !layout_object->Style())
    return nullptr;

  // DO NOT SUBMIT: have to clone the gradient values or something
  // to avoid the const cast.
  return const_cast<cssvalue::CSSGradientValue&>(*value_).GetImage(
      *layout_object, node_->GetDocument(), layout_object->StyleRef(),
      default_object_size);
}

void CSSStyleGradientValue::Trace(blink::Visitor* visitor) {
  visitor->Trace(value_);
  visitor->Trace(node_);
  CSSResourceValue::Trace(visitor);
}

const CSSValue* CSSStyleGradientValue::ToCSSValue() const {
  return value_.Get();
}

}  // namespace blink
