// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_SINGLE_RELEASE_CALLBACK_H_
#define CC_RESOURCES_SINGLE_RELEASE_CALLBACK_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "cc/cc_export.h"

namespace gpu {
struct SyncToken;
}  // namespace gpu

namespace cc {

class CC_EXPORT SingleReleaseCallback {
 public:
  using CallbackType =
      base::OnceCallback<void(const gpu::SyncToken& sync_token, bool is_lost)>;

  static std::unique_ptr<SingleReleaseCallback> Create(CallbackType cb) {
    return base::WrapUnique(new SingleReleaseCallback(std::move(cb)));
  }

  ~SingleReleaseCallback();

  void Run(const gpu::SyncToken& sync_token, bool is_lost);

 private:
  explicit SingleReleaseCallback(CallbackType callback);

  CallbackType callback_;
};

}  // namespace cc

#endif  // CC_RESOURCES_SINGLE_RELEASE_CALLBACK_H_
