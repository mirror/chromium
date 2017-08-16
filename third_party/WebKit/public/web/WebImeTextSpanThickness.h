// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebImeTextSpanThickness_h
#define WebImeTextSpanThickness_h

namespace blink {

enum WebImeTextSpanThickness {
  kWebImeTextSpanThicknessNone,
  kWebImeTextSpanThicknessThin,
  kWebImeTextSpanThicknessThick,
  kWebImeTextSpanThicknessLast = kWebImeTextSpanThicknessThick
};

}  // namespace blink

#endif
