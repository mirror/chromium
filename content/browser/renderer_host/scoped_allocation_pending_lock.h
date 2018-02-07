// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_SCOPED_ALLOCATION_PENDING_LOCK_H_
#define CONTENT_BROWSER_RENDERER_HOST_SCOPED_ALLOCATION_PENDING_LOCK_H_

namespace content {
class RenderWidgetHostViewBase;

class ScopedAllocationPendingLock {
 public:
  explicit ScopedAllocationPendingLock(const RenderWidgetHostViewBase* view);

  ScopedAllocationPendingLock(const ScopedAllocationPendingLock& other) =
      delete;
  ScopedAllocationPendingLock& operator=(
      const ScopedAllocationPendingLock& other) = delete;
  ScopedAllocationPendingLock(ScopedAllocationPendingLock&& other) = default;
  ScopedAllocationPendingLock& operator=(ScopedAllocationPendingLock&& other) =
      default;

  ~ScopedAllocationPendingLock();

 private:
  const RenderWidgetHostViewBase* view_;
  viz::LocalSurfaceId local_surface_id_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_SCOPED_ALLOCATION_PENDING_LOCK_H_
