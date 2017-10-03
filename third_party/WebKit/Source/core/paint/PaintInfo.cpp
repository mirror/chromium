// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/PaintInfo.h"
#include "core/html/HTMLImageElement.h"
#include "core/svg/SVGImageElement.h"

namespace blink {

Image::ImageDecodingMode GetImageDecodingMode(Node* node) {
  if (!RuntimeEnabledFeatures::ImageAsyncAttributeEnabled())
    return Image::kUnspecifiedDecode;

  AtomicString async_attr;
  if (IsHTMLImageElement(node)) {
    async_attr =
        ToHTMLImageElement(node)->FastGetAttribute(HTMLNames::asyncAttr);
  } else if (IsSVGImageElement(node)) {
    async_attr = ToSVGImageElement(node)->FastGetAttribute(SVGNames::asyncAttr);
  }

  if (async_attr.IsNull())
    return Image::kUnspecifiedDecode;
  if (async_attr.LowerASCII() == "sync")
    return Image::kSyncDecode;
  if (async_attr == "" || async_attr.LowerASCII() == "async")
    return Image::kAsyncDecode;
  return Image::kUnspecifiedDecode;
}

}  // namespace blink
