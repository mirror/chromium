// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_OVERSCROLL_UI_SCROLL_INPUT_MANAGER_H_
#define UI_COMPOSITOR_OVERSCROLL_UI_SCROLL_INPUT_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/trees/element_id.h"
#include "ui/compositor/compositor_export.h"

namespace cc {
class InputHandler;
}

namespace ui {

class Layer;
class ScrollEvent;
class UIInputHandler;

// UI-Thread helper for directing scroll events to the Compositor to enable
// accelerated scrolling, and elastic scrolling on Mac.
class COMPOSITOR_EXPORT UIScrollInputManager {
 public:
  explicit UIScrollInputManager(
      const base::WeakPtr<cc::InputHandler>& input_handler);
  ~UIScrollInputManager();

  // Scroll the given |layer_to_scroll| according to |event|.
  bool OnScrollEvent(const ScrollEvent& event, Layer* layer_to_scroll);

 private:
  std::unique_ptr<UIInputHandler> input_wrapper_;

  DISALLOW_COPY_AND_ASSIGN(UIScrollInputManager);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_OVERSCROLL_UI_SCROLL_INPUT_MANAGER_H_
