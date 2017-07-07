// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerMediaPlayer.h"

namespace blink {

WorkerMediaPlayer::WorkerMediaPlayer(ExecutionContext* context)
    : ContextLifecycleObserver(context) {}

ExecutionContext* WorkerMediaPlayer::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

DEFINE_TRACE(WorkerMediaPlayer) {
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
