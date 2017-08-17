// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IME_TEXT_SPAN_THICKNESS_H_
#define IME_TEXT_SPAN_THICKNESS_H_

namespace ui {

enum ImeTextSpanThickness {
  IME_TEXT_SPAN_THICKNESS_NONE,
  IME_TEXT_SPAN_THICKNESS_THIN,
  IME_TEXT_SPAN_THICKNESS_THICK,

  IME_TEXT_SPAN_THICKNESS_MAX = IME_TEXT_SPAN_THICKNESS_THICK
};

}  // namespace ui

#endif  // IME_TEXT_SPAN_THICKNESS_H_
