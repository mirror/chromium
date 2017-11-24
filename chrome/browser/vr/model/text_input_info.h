// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_TEXT_INPUT_INFO_H_
#define CHROME_BROWSER_VR_MODEL_TEXT_INPUT_INFO_H_

#include "base/strings/string16.h"

namespace vr {

struct TextInputInfo {
  // The value of the currently focused input field.
  base::string16 text;

  // The cursor position of the current selection start, or the caret position
  // if nothing is selected.
  int selection_start;

  // The cursor position of the current selection end, or the caret position
  // if nothing is selected.
  int selection_end;

  bool Equals(const TextInputInfo& m) const {
    return text == m.text && selection_start == m.selection_start &&
           selection_end == m.selection_end;
  }

  TextInputInfo() : selection_start(0), selection_end(0) {}
  explicit TextInputInfo(base::string16 t)
      : text(t), selection_start(t.length()), selection_end(t.length()) {}
};

inline bool operator==(const TextInputInfo& a, const TextInputInfo& b) {
  return a.Equals(b);
}

inline bool operator!=(const TextInputInfo& a, const TextInputInfo& b) {
  return !(a == b);
}

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_TEXT_INPUT_INFO_H_
