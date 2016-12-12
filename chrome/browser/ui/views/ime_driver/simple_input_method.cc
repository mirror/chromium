// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/ime_driver/simple_input_method.h"

SimpleInputMethod::SimpleInputMethod() {}

SimpleInputMethod::~SimpleInputMethod() {}

void SimpleInputMethod::OnTextInputModeChanged(
    ui::mojom::TextInputMode text_input_mode) {}

void SimpleInputMethod::OnTextInputTypeChanged(
    ui::mojom::TextInputType text_input_type) {}

void SimpleInputMethod::OnCaretBoundsChanged(const gfx::Rect& caret_bounds) {}

void SimpleInputMethod::ProcessKeyEvent(
    std::unique_ptr<ui::Event> key_event,
    const ProcessKeyEventCallback& callback) {
  callback.Run(false);
}

void SimpleInputMethod::CancelComposition() {}
