// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerAudioPlayer.h"

#include "core/EventTargetNames.h"

namespace blink {

WorkerAudioPlayer::WorkerAudioPlayer(ExecutionContext* context)
    : WorkerMediaPlayer(context) {}

// static
WorkerAudioPlayer* WorkerAudioPlayer::CreateForJSConstructor(
    ExecutionContext* context,
    const String& src) {
  WorkerAudioPlayer* audio = new WorkerAudioPlayer(context);
  audio->setPreload("auto");
  if (!src.IsNull())
    audio->setSrc(src);
  return audio;
}

const AtomicString& WorkerAudioPlayer::InterfaceName() const {
  return EventTargetNames::WorkerAudioPlayer;
}

}  // namespace blink
