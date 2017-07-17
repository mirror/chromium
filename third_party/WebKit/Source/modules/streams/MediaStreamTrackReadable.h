// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaStreamTrackReadable_h
#define MediaStreamTrackReadable_h

#include "platform/wtf/Allocator.h"

namespace blink {

class ScriptState;
class ScriptValue;
class MediaStreamTrack;

class MediaStreamTrackReadable {
  STATIC_ONLY(MediaStreamTrackReadable);

 public:
  static ScriptValue readable(ScriptState*, MediaStreamTrack&);
};

}  // namespace blink

#endif  // MediaStreamTrackReadable_h
