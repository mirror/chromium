// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/streams/MediaStreamTrackReadable.h"

#include "core/streams/ReadableStreamOperations.h"
#include "modules/mediastream/MediaStreamTrack.h"
#include "modules/streams/MediaStreamTrackSource.h"

namespace blink {

// static
ScriptValue MediaStreamTrackReadable::readable(ScriptState* script_state,
                                               MediaStreamTrack& track) {
  return ReadableStreamOperations::CreateReadableStream(
      script_state, new MediaStreamTrackSource(script_state, track),
      ReadableStreamOperations::CreateCountQueuingStrategy(script_state, 0));
}

}  // namespace blink
