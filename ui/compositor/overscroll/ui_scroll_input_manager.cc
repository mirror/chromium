// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/overscroll/ui_scroll_input_manager.h"

#include "base/logging.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/overscroll/ui_input_handler.h"
#include "ui/events/event.h"

namespace ui {

UIScrollInputManager::UIScrollInputManager(
    const base::WeakPtr<cc::InputHandler>& input_handler)
    : input_wrapper_(new UIInputHandler(input_handler)) {}

UIScrollInputManager::~UIScrollInputManager() {}

bool UIScrollInputManager::OnScrollEvent(const ScrollEvent& event,
                                         Layer* layer_to_scroll) {
  // Note there is no "begin". Currently it's only needed for hit testing,
  // but UI already knows which layer should be scrolling.
  input_wrapper_->HandleScrollUpdate(event, layer_to_scroll->GetElementId());
  if (event.momentum_phase() == EventMomentumPhase::END)
    input_wrapper_->HandleScrollEnd(event);
  return true;
}

}  // namespace ui