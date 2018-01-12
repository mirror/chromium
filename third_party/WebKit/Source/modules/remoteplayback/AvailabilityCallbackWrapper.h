// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AvailabilityCallbackWrapper_h
#define AvailabilityCallbackWrapper_h

#include "base/callback.h"
#include "bindings/modules/v8/v8_remote_playback_availability_callback.h"
#include "platform/heap/Handle.h"

namespace blink {

class RemotePlayback;

// Wraps either a base::OnceClosure or RemotePlaybackAvailabilityCallback object
// to be kept in the RemotePlayback's |availability_callbacks_| map.
class AvailabilityCallbackWrapper final
    : public GarbageCollectedFinalized<AvailabilityCallbackWrapper> {
  WTF_MAKE_NONCOPYABLE(AvailabilityCallbackWrapper);

 public:
  explicit AvailabilityCallbackWrapper(V8RemotePlaybackAvailabilityCallback*);
  explicit AvailabilityCallbackWrapper(base::RepeatingClosure);
  ~AvailabilityCallbackWrapper() = default;

  void Run(RemotePlayback*, bool new_availability);

  void Trace(blink::Visitor*) {}

 private:
  // Only one of these callbacks must be set.
  V8RemotePlaybackAvailabilityCallback::Persistent<
      V8RemotePlaybackAvailabilityCallback>
      bindings_cb_;
  base::RepeatingClosure internal_cb_;
};

}  // namespace blink

#endif  // AvailabilityCallbackWrapper_h
