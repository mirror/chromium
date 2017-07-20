// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletObjectProxy.h"

#include "core/workers/ThreadedWorkletMessagingProxy.h"
#include "modules/webaudio/AudioWorkletMessagingProxy.h"

namespace blink {

AudioWorkletObjectProxy::AudioWorkletObjectProxy(
    AudioWorkletMessagingProxy* messaging_proxy_weak_ptr,
    ParentFrameTaskRunners* parent_frame_task_runners)
    : ThreadedWorkletObjectProxy(
          static_cast<ThreadedWorkletMessagingProxy*>(messaging_proxy_weak_ptr),
          parent_frame_task_runners) {}

}  // namespace blink
