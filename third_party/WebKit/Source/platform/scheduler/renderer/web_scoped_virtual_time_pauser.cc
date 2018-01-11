// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebScopedVirtualTimePauser.h"

#include "platform/scheduler/renderer/renderer_scheduler_impl.h"

namespace blink {

WebScopedVirtualTimePauser::WebScopedVirtualTimePauser()
    : scheduler_(nullptr) {}

WebScopedVirtualTimePauser::WebScopedVirtualTimePauser(
    scheduler::RendererSchedulerImpl* scheduler)
    : scheduler_(scheduler) {}

WebScopedVirtualTimePauser::~WebScopedVirtualTimePauser() {
  if (paused_ && scheduler_)
    DecrementVirtualTimePauseCount();
}

WebScopedVirtualTimePauser::WebScopedVirtualTimePauser(
    WebScopedVirtualTimePauser&& other) {
  virtual_time_when_paused_ = other.virtual_time_when_paused_;
  paused_ = other.paused_;
  scheduler_ = std::move(other.scheduler_);
  other.scheduler_ = nullptr;
}

WebScopedVirtualTimePauser& WebScopedVirtualTimePauser::operator=(
    WebScopedVirtualTimePauser&& other) {
  if (scheduler_ && paused_)
    DecrementVirtualTimePauseCount();
  virtual_time_when_paused_ = other.virtual_time_when_paused_;
  paused_ = other.paused_;
  scheduler_ = std::move(other.scheduler_);
  other.scheduler_ = nullptr;
  return *this;
}

void WebScopedVirtualTimePauser::PauseVirtualTime(bool paused) {
  if (paused == paused_ || !scheduler_)
    return;

  paused_ = paused;
  if (paused_) {
    virtual_time_when_paused_ = scheduler_->IncrementVirtualTimePauseCount();
  } else {
    DecrementVirtualTimePauseCount();
  }
}

void WebScopedVirtualTimePauser::DecrementVirtualTimePauseCount() {
  scheduler_->DecrementVirtualTimePauseCount();
  scheduler_->MaybeAdvanceVirtualTime(virtual_time_when_paused_ +
                                      base::TimeDelta::FromMilliseconds(10));
}

}  // namespace blink
