// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_SINGLE_RELEASE_CALLBACK_IMPL_H_
#define COMPONENTS_VIZ_COMMON_QUADS_SINGLE_RELEASE_CALLBACK_IMPL_H_

#include <memory>

#include "base/memory/ptr_util.h"
#include "components/viz/common/quads/release_callback_impl.h"
#include "components/viz/common/viz_common_export.h"

namespace viz {

class VIZ_COMMON_EXPORT SingleReleaseCallbackImpl {
 public:
  static std::unique_ptr<SingleReleaseCallbackImpl> Create(
      const ReleaseCallbackImpl& cb) {
    return base::WrapUnique(new SingleReleaseCallbackImpl(cb));
  }

  ~SingleReleaseCallbackImpl();

  void Run(const gpu::SyncToken& sync_token,
           bool is_lost,
           cc::BlockingTaskRunner* main_thread_task_runner);

 private:
  explicit SingleReleaseCallbackImpl(const ReleaseCallbackImpl& callback);

  ReleaseCallbackImpl callback_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_SINGLE_RELEASE_CALLBACK_IMPL_H_
