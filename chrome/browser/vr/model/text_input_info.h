// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_TEXT_INPUT_INFO_H_
#define CHROME_BROWSER_VR_MODEL_TEXT_INPUT_INFO_H_

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"

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

  // The start position of the current composition, or -1 if there is none.
  int composition_start;

  // The end position of the current composition, or -1 if there is none.
  int composition_end;

  inline bool operator==(TextInputInfo m) {
    return text == m.text && selection_start == m.selection_start &&
           selection_end == m.selection_end &&
           composition_start == m.composition_start &&
           composition_end == m.composition_end;
  }

  TextInputInfo()
      : selection_start(0),
        selection_end(0),
        composition_start(-1),
        composition_end(-1) {}

  std::string DebugString() const {
    return "text: " + base::UTF16ToUTF8(text) + " selection(" +
           std::to_string(selection_start) + ", " +
           std::to_string(selection_end) + ") composing (" +
           std::to_string(composition_start) + ", " +
           std::to_string(composition_end) + ")";
  }
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_TEXT_INPUT_INFO_H_
