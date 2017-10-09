// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontFormatCheck_h
#define FontFormatCheck_h

#include "platform/wtf/Vector.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace blink {

class FontFormatCheck {
 public:
  FontFormatCheck(sk_sp<SkTypeface>);
  bool IsVariableFont();
  bool IsCbdtCblcColorFont();
  bool IsSbixColorFont();
  bool IsCff2OutlineFont();

 private:
  Vector<SkFontTableTag> table_tags_;
};

}  // namespace blink

#endif
