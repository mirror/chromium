// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_RELEASE_CALLBACK_IMPL_H_
#define COMPONENTS_VIZ_COMMON_QUADS_RELEASE_CALLBACK_IMPL_H_

#include "base/callback.h"

namespace gpu {
struct SyncToken;
}

namespace cc {
class BlockingTaskRunner;
}

namespace viz {

typedef base::Callback<void(const gpu::SyncToken& sync_token,
                            bool is_lost,
                            cc::BlockingTaskRunner* main_thread_task_runner)>
    ReleaseCallbackImpl;

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_RELEASE_CALLBACK_IMPL_H_
