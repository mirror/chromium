// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SCHEDULER_RENDERER_RENDERER_WEB_SCHEDULER_IMPL_H_
#define COMPONENTS_SCHEDULER_RENDERER_RENDERER_WEB_SCHEDULER_IMPL_H_

#include "components/scheduler/child/web_scheduler_impl.h"

namespace scheduler {

class RendererScheduler;

class SCHEDULER_EXPORT RendererWebSchedulerImpl : public WebSchedulerImpl {
 public:
  explicit RendererWebSchedulerImpl(RendererScheduler* child_scheduler);

  ~RendererWebSchedulerImpl() override;

  // blink::WebScheduler implementation:
  void suspendTimerQueue() override;
  void resumeTimerQueue() override;
  blink::WebFrameHostScheduler* createFrameHostScheduler() override;
  void addPendingNavigation() override;
  void removePendingNavigation() override;
  void onNavigationStarted() override;

 private:
  RendererScheduler* renderer_scheduler_;  // NOT OWNED
};

}  // namespace scheduler

#endif  // COMPONENTS_SCHEDULER_RENDERER_RENDERER_WEB_SCHEDULER_IMPL_H_
