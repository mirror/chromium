// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef StyleBuilderUtils_h
#define StyleBuilderUtils_h

#include "core/style/BorderImageLength.h"
#include "core/style/BorderImageLengthBox.h"
#include "platform/Length.h"
#include "platform/LengthBox.h"

namespace blink {

class StyleBuilderUtils {
  STATIC_ONLY(StyleBuilderUtils);

 public:
  static bool borderImageLengthMatchesAllSides(
      const BorderImageLengthBox& borderImageLengthBox,
      const BorderImageLength& borderImageLength) {
    return (borderImageLengthBox.Left() == borderImageLength &&
            borderImageLengthBox.Right() == borderImageLength &&
            borderImageLengthBox.Top() == borderImageLength &&
            borderImageLengthBox.Bottom() == borderImageLength);
  }
  static bool lengthMatchesAllSides(const LengthBox& lengthBox,
                                    const Length& length) {
    return (lengthBox.Left() == length && lengthBox.Right() == length &&
            lengthBox.Top() == length && lengthBox.Bottom() == length);
  }
};

}  // namespace blink

#endif  // StyleBuilderUtils_h
