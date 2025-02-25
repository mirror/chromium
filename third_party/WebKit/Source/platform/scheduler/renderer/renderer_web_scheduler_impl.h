// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_RENDERER_WEB_SCHEDULER_IMPL_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_RENDERER_WEB_SCHEDULER_IMPL_H_

#include "platform/scheduler/child/web_scheduler_impl.h"

namespace blink {
namespace scheduler {

class RendererSchedulerImpl;

class PLATFORM_EXPORT RendererWebSchedulerImpl : public WebSchedulerImpl {
 public:
  explicit RendererWebSchedulerImpl(RendererSchedulerImpl* renderer_scheduler);

  ~RendererWebSchedulerImpl() override;

  // WebScheduler implementation:
  base::SingleThreadTaskRunner* CompositorTaskRunner() override;
  std::unique_ptr<RendererPauseHandle> PauseScheduler() override
      WARN_UNUSED_RESULT;
  std::unique_ptr<WebViewScheduler> CreateWebViewScheduler(
      InterventionReporter* intervention_reporter,
      WebViewScheduler::WebViewSchedulerDelegate* delegate) override;

  base::TimeTicks MonotonicallyIncreasingVirtualTime() const override;

  RendererScheduler* GetRendererSchedulerForTest() override;

 private:
  RendererSchedulerImpl* renderer_scheduler_;  // NOT OWNED
  scoped_refptr<WebTaskRunnerImpl> compositor_task_runner_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_RENDERER_WEB_SCHEDULER_IMPL_H_
