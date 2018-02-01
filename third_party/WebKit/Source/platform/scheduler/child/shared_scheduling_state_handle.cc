// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/shared_scheduling_state_handle.h"

namespace blink {
namespace scheduler {

SharedSchedulingStateHandle::SharedSchedulingStateHandle(
    scoped_refptr<internal::SharedSchedulingState> state)
    : state_(state) {}

SharedSchedulingStateHandle::~SharedSchedulingStateHandle() {
  state_->OnHandleDestroyed();
}

scoped_refptr<internal::SharedSchedulingState>
SharedSchedulingStateHandle::Get() {
  return state_;
}

}  // namespace scheduler
}  // namespace blink
