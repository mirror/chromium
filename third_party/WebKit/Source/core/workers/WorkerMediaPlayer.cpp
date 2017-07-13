// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerMediaPlayer.h"

namespace blink {

WorkerMediaPlayer::WorkerMediaPlayer(ExecutionContext* context)
    : media_player_(new MediaPlayer(context, this)) {}

ExecutionContext* WorkerMediaPlayer::GetExecutionContext() const {
  return media_player_->GetExecutionContext();
}

DEFINE_TRACE(WorkerMediaPlayer) {
  visitor->Trace(media_player_);

  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
