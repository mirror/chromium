// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_SINGLE_RELEASE_CALLBACK_H_
#define CC_RESOURCES_SINGLE_RELEASE_CALLBACK_H_

#include "base/callback_forward.h"

namespace gpu {
struct SyncToken;
}

namespace cc {

using SingleReleaseCallback =
    base::OnceCallback<void(const gpu::SyncToken& sync_token, bool is_lost)>;

}  // namespace cc

#endif  // CC_RESOURCES_SINGLE_RELEASE_CALLBACK_H_
