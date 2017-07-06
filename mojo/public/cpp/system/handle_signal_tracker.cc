// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/system/handle_signal_tracker.h"

#include "base/synchronization/lock.h"

namespace mojo {

HandleSignalTracker::HandleSignalTracker(Handle handle,
                                         MojoHandleSignals signals)
    : high_watcher_(FROM_HERE, SimpleWatcher::ArmingPolicy::MANUAL),
      low_watcher_(FROM_HERE, SimpleWatcher::ArmingPolicy::MANUAL) {
  high_watcher_.Watch(
      handle, signals, MOJO_WATCH_CONDITION_SATISFIED,
      base::Bind(&HandleSignalTracker::OnNotify, base::Unretained(this)));
  low_watcher_.Watch(
      handle, signals, MOJO_WATCH_CONDITION_NOT_SATISFIED,
      base::Bind(&HandleSignalTracker::OnNotify, base::Unretained(this)));
}

HandleSignalTracker::~HandleSignalTracker() = default;

void HandleSignalTracker::OnNotify(MojoResult result,
                                   const HandleSignalsState& state) {
  last_known_state_ = state;
}

}  // namespace mojo