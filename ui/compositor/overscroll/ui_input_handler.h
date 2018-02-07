// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_OVERSCROLL_UI_INPUT_HANLDER_H_
#define UI_COMPOSITOR_OVERSCROLL_UI_INPUT_HANLDER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/input/input_handler.h"
#include "ui/events/event.h"

namespace ui {

// Class to feed UI-thread scroll events to a cc::InputHandler. Inspired by
// ui::InputHandlerProxy but greatly simplified.
class UIInputHandler : public cc::InputHandlerClient {
 public:
  explicit UIInputHandler(const base::WeakPtr<cc::InputHandler>& input_handler);
  ~UIInputHandler() override;

  // Ask the InputHandler to scroll |element| according to |scroll|.
  void HandleScrollUpdate(const ScrollEvent& scroll, cc::ElementId element);

  // Informs the InputHandler of the end of the event stream.
  void HandleScrollEnd(const ScrollEvent& scroll);

  // cc::InputHandlerClient implementation.
  void WillShutdown() override;
  void Animate(base::TimeTicks time) override;
  void MainThreadHasStoppedFlinging() override;
  void ReconcileElasticOverscrollAndRootScroll() override;
  void UpdateRootLayerStateForSynchronousInputHandler(
      const gfx::ScrollOffset& total_scroll_offset,
      const gfx::ScrollOffset& max_scroll_offset,
      const gfx::SizeF& scrollable_size,
      float page_scale_factor,
      float min_page_scale_factor,
      float max_page_scale_factor) override;
  void DeliverInputForBeginFrame() override;

 private:
  cc::InputHandler* input_handler_;  // Weak. Cleared in WillShutdown().

  DISALLOW_COPY_AND_ASSIGN(UIInputHandler);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_OVERSCROLL_UI_INPUT_HANLDER_H_