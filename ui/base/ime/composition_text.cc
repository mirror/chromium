// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/composition_text.h"

namespace ui {

CompositionText::CompositionText() {
}

CompositionText::CompositionText(const CompositionText& other) = default;

CompositionText::~CompositionText() {
}

void CompositionText::Clear() {
  text.clear();
  text_composition_data = TextCompositionData();
  selection = gfx::Range();
}

void CompositionText::CopyFrom(const CompositionText& obj) {
  Clear();
  text = obj.text;
  text_composition_data = obj.text_composition_data;
  selection = obj.selection;
}

}  // namespace ui
