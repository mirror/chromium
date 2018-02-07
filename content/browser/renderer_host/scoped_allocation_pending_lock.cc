// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_base.h"

namespace content {

ScopedAllocationPendingLock::ScopedAllocationPendingLock(
    const RenderWidgetHostViewBase* view)
    : view_(view), local_surface_id_(view->GetLocalSurfaceId()) {
  // TODO: If you hit this DCHECK, it is because this is not ref counted
  // // and you are attempting to allow multiple locks in flight at the
  // // same time. You'll need to add ref counting.
  DCHECK(!view->IsAllocationPending());
  view->SetAllocationPendingFlag();
}

ScopedAllocationPendingLock::~ScopedAllocationPendingLock() {
  DCHECK(view_->GetLocalSurfaceId() == local_surface_id_);
  view_->ClearAllocationPendingFlag();
}

}  // namespace content
