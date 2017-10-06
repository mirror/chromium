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
  if (IsHTMLImageElement(node))
    return ToHTMLImageElement(node)->GetDecodingMode();
  if (IsSVGImageElement(node))
    return ToSVGImageElement(node)->GetDecodingMode();
  return Image::kUnspecifiedDecode;
}

}  // namespace blink
