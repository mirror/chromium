// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_SINGLE_RELEASE_CALLBACK_IMPL_H_
#define CC_RESOURCES_SINGLE_RELEASE_CALLBACK_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "cc/cc_export.h"

namespace gpu {
struct SyncToken;
}  // namespace gpu

namespace cc {
class BlockingTaskRunner;

class CC_EXPORT SingleReleaseCallbackImpl {
 public:
  using CallbackType =
      base::OnceCallback<void(const gpu::SyncToken& sync_token,
                              bool is_lost,
                              BlockingTaskRunner* main_thread_task_runner)>;

  static std::unique_ptr<SingleReleaseCallbackImpl> Create(CallbackType cb) {
    return base::WrapUnique(new SingleReleaseCallbackImpl(std::move(cb)));
  }

  ~SingleReleaseCallbackImpl();

  void Run(const gpu::SyncToken& sync_token,
           bool is_lost,
           BlockingTaskRunner* main_thread_task_runner);

 private:
  explicit SingleReleaseCallbackImpl(CallbackType callback);

  CallbackType callback_;
};

}  // namespace cc

#endif  // CC_RESOURCES_SINGLE_RELEASE_CALLBACK_IMPL_H_
