// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerAudioPlayer_h
#define WorkerAudioPlayer_h

#include "core/CoreExport.h"
#include "core/workers/WorkerMediaPlayer.h"

namespace blink {

class CORE_EXPORT WorkerAudioPlayer : public WorkerMediaPlayer {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static WorkerAudioPlayer* CreateForJSConstructor(ExecutionContext*,
                                                   const String& src);

  // WorkerMediaPlayer
  const AtomicString& InterfaceName() const override;

 private:
  WorkerAudioPlayer(ExecutionContext*);
};

}  // namespace blink

#endif  // WorkerAudioPlayer_h
