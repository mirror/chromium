// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/overscroll/ui_input_handler.h"

#include "base/logging.h"
#include "cc/trees/layer_tree_impl.h"
#include "cc/trees/scroll_node.h"
#include "ui/events/event.h"

namespace ui {

namespace {

// Creates a cc::ScrollState from a ui::ScrollEvent, populating fields general
// to all event phases. Take care not to put deltas on beginning/end updates,
// since InputHandler will DCHECK if they're present.
cc::ScrollState CreateScrollState(const ScrollEvent& scroll, bool is_end) {
  cc::ScrollStateData scroll_state_data;
  scroll_state_data.position_x = scroll.x();
  scroll_state_data.position_y = scroll.y();
  if (!is_end) {
    scroll_state_data.delta_x = -scroll.x_offset_ordinal();
    scroll_state_data.delta_y = -scroll.y_offset_ordinal();
  }
  scroll_state_data.is_in_inertial_phase =
      scroll.momentum_phase() == EventMomentumPhase::INERTIAL_UPDATE;
  scroll_state_data.is_ending = is_end;
  return cc::ScrollState(scroll_state_data);
}

}  // namespace

UIInputHandler::UIInputHandler(
    const base::WeakPtr<cc::InputHandler>& input_handler)
    : input_handler_(input_handler.get()) {
  input_handler_->BindToClient(this, false);
}

UIInputHandler::~UIInputHandler() {
  DCHECK(!input_handler_);
}

void UIInputHandler::WillShutdown() {
  input_handler_ = nullptr;
}

void UIInputHandler::Animate(base::TimeTicks time) {}

void UIInputHandler::MainThreadHasStoppedFlinging() {}

void UIInputHandler::ReconcileElasticOverscrollAndRootScroll() {}

void UIInputHandler::UpdateRootLayerStateForSynchronousInputHandler(
    const gfx::ScrollOffset& total_scroll_offset,
    const gfx::ScrollOffset& max_scroll_offset,
    const gfx::SizeF& scrollable_size,
    float page_scale_factor,
    float min_page_scale_factor,
    float max_page_scale_factor) {}

void UIInputHandler::DeliverInputForBeginFrame() {}

void UIInputHandler::HandleScrollUpdate(const ScrollEvent& scroll,
                                        cc::ElementId element) {
  cc::ScrollState scroll_state = CreateScrollState(scroll, false);
  scroll_state.data()->set_current_native_scrolling_element(element);
  input_handler_->ScrollBy(&scroll_state);
}

void UIInputHandler::HandleScrollEnd(const ScrollEvent& scroll) {
  cc::ScrollState scroll_state = CreateScrollState(scroll, true);
  input_handler_->ScrollEnd(&scroll_state, false /* for now */);
}

}  // namespace ui