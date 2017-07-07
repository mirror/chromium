// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerMediaPlayer_h
#define WorkerMediaPlayer_h

#include "core/CoreExport.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "core/events/EventTarget.h"
#include "core/media/MediaPlayer.h"

namespace blink {

class CORE_EXPORT WorkerMediaPlayer : public EventTargetWithInlineData,
                                      public ContextLifecycleObserver,
                                      public MediaPlayer {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(WorkerMediaPlayer);

 public:
  // EventTarget
  ExecutionContext* GetExecutionContext() const final;

  DECLARE_VIRTUAL_TRACE();

 protected:
  WorkerMediaPlayer(ExecutionContext*);
};

}  // namespace blink

#endif  // WorkerMediaPlayer_h
