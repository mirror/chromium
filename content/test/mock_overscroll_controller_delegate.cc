// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_overscroll_controller_delegate.h"

#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/public/test/test_utils.h"
#include "ui/display/screen.h"

namespace content {

MockOverscrollControllerDelegate::MockOverscrollControllerDelegate(
    RenderWidgetHostViewAura* rwhva)
    : rwhva_(rwhva),
      update_message_loop_runner_(new MessageLoopRunner),
      end_message_loop_runner_(new MessageLoopRunner),
      seen_update_(false),
      overscroll_ended_(false) {}

MockOverscrollControllerDelegate::~MockOverscrollControllerDelegate() {}

gfx::Size MockOverscrollControllerDelegate::GetDisplaySize() const {
  return display::Screen::GetScreen()
      ->GetDisplayNearestView(rwhva_->GetNativeView())
      .size();
}

base::Optional<float> MockOverscrollControllerDelegate::GetMaxOverscrollDelta()
    const {
  return base::nullopt;
}

bool MockOverscrollControllerDelegate::OnOverscrollUpdate(float, float) {
  seen_update_ = true;
  if (update_message_loop_runner_->loop_running())
    update_message_loop_runner_->Quit();
  return true;
}

void MockOverscrollControllerDelegate::OnOverscrollComplete(OverscrollMode) {
  OnOverscrollEnd();
}

void MockOverscrollControllerDelegate::OnOverscrollModeChange(
    OverscrollMode old_mode,
    OverscrollMode new_mode,
    OverscrollSource source) {
  if (new_mode == OVERSCROLL_NONE)
    OnOverscrollEnd();
}

void MockOverscrollControllerDelegate::WaitForUpdate() {
  if (!seen_update_)
    update_message_loop_runner_->Run();
}

void MockOverscrollControllerDelegate::WaitForEnd() {
  if (!overscroll_ended_)
    end_message_loop_runner_->Run();
}

void MockOverscrollControllerDelegate::Reset() {
  update_message_loop_runner_ = new MessageLoopRunner;
  end_message_loop_runner_ = new MessageLoopRunner;
  seen_update_ = false;
  overscroll_ended_ = false;
}

void MockOverscrollControllerDelegate::OnOverscrollEnd() {
  overscroll_ended_ = true;
  if (end_message_loop_runner_->loop_running())
    end_message_loop_runner_->Quit();
}

}  // namespace content
