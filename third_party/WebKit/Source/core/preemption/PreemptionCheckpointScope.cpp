// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/preemption/PreemptionCheckpointScope.h"

#include "core/preemption/PreemptionManager.h"
#include "platform/bindings/V8PerIsolateData.h"

namespace blink {

PreemptionCheckpointScope::PreemptionCheckpointScope(v8::Isolate* isolate) {
  auto* manager = static_cast<PreemptionManager*>(
      V8PerIsolateData::From(isolate)->PreemptionManager());
  if (!manager)
    return;

  preemption_manager_ = manager->GetWeakPtr();
  old_value_ = preemption_manager_->InPreemptionCheckpointScope();
  if (!old_value_)
    preemption_manager_->EnterPreemptionCheckpointScope();
}

PreemptionCheckpointScope::~PreemptionCheckpointScope() {
  if (preemption_manager_ && !old_value_)
    preemption_manager_->LeavePreemptionCheckpointScope();
}

}  // namespace blink
