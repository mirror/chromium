// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/renderer/main_thread_task_queue.h"

#include "platform/scheduler/base/task_queue_impl.h"
#include "platform/scheduler/renderer/renderer_scheduler_impl.h"

namespace blink {
namespace scheduler {

// static
const char* MainThreadTaskQueue::ReportingMessageForQueueType(
    MainThreadTaskQueue::QueueType queue_type) {
  switch (queue_type) {
    case MainThreadTaskQueue::QueueType::CONTROL:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType.Control";
    case MainThreadTaskQueue::QueueType::DEFAULT:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType.Default";
    case MainThreadTaskQueue::QueueType::DEFAULT_LOADING:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "DefaultLoading";
    case MainThreadTaskQueue::QueueType::DEFAULT_TIMER:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "DefaultTimer";
    case MainThreadTaskQueue::QueueType::UNTHROTTLED:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "Unthrottled";
    case MainThreadTaskQueue::QueueType::FRAME_LOADING:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "FrameLoading";
    case MainThreadTaskQueue::QueueType::FRAME_THROTTLEABLE:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "FrameThrottleable";
    case MainThreadTaskQueue::QueueType::FRAME_DEFERRABLE:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "FrameDeferrable";
    case MainThreadTaskQueue::QueueType::FRAME_PAUSABLE:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "FramePausable";
    case MainThreadTaskQueue::QueueType::FRAME_UNPAUSABLE:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "FrameUnpausable";
    case MainThreadTaskQueue::QueueType::COMPOSITOR:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType.Compositor";
    case MainThreadTaskQueue::QueueType::IDLE:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType.Idle";
    case MainThreadTaskQueue::QueueType::TEST:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType.Test";
    case MainThreadTaskQueue::QueueType::FRAME_LOADING_CONTROL:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType."
             "FrameLoadingControl";
    case MainThreadTaskQueue::QueueType::V8:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType.V8";
    case MainThreadTaskQueue::QueueType::OTHER:
      return "RendererScheduler.ExpectedQueueingTimeByTaskQueueType.Other";
    case MainThreadTaskQueue::QueueType::COUNT:
      NOTREACHED();
      return nullptr;
  }
  NOTREACHED();
  return nullptr;
}

MainThreadTaskQueue::QueueClass MainThreadTaskQueue::QueueClassForQueueType(
    QueueType type) {
  switch (type) {
    case QueueType::CONTROL:
    case QueueType::DEFAULT:
    case QueueType::IDLE:
    case QueueType::TEST:
    case QueueType::V8:
      return QueueClass::NONE;
    case QueueType::DEFAULT_LOADING:
    case QueueType::FRAME_LOADING:
    case QueueType::FRAME_LOADING_CONTROL:
      return QueueClass::LOADING;
    case QueueType::DEFAULT_TIMER:
    case QueueType::UNTHROTTLED:
    case QueueType::FRAME_THROTTLEABLE:
    case QueueType::FRAME_DEFERRABLE:
    case QueueType::FRAME_PAUSABLE:
    case QueueType::FRAME_UNPAUSABLE:
      return QueueClass::TIMER;
    case QueueType::COMPOSITOR:
      return QueueClass::COMPOSITOR;
    case QueueType::OTHER:
    case QueueType::COUNT:
      DCHECK(false);
      return QueueClass::COUNT;
  }
  NOTREACHED();
  return QueueClass::NONE;
}

MainThreadTaskQueue::MainThreadTaskQueue(
    std::unique_ptr<internal::TaskQueueImpl> impl,
    const QueueCreationParams& params,
    RendererSchedulerImpl* renderer_scheduler)
    : TaskQueue(std::move(impl)),
      queue_type_(params.queue_type),
      queue_class_(QueueClassForQueueType(params.queue_type)),
      can_be_blocked_(params.can_be_blocked),
      can_be_throttled_(params.can_be_throttled),
      can_be_paused_(params.can_be_paused),
      can_be_stopped_(params.can_be_stopped),
      used_for_control_tasks_(params.used_for_control_tasks),
      renderer_scheduler_(renderer_scheduler),
      web_frame_scheduler_(nullptr) {
  if (GetTaskQueueImpl()) {
    // TaskQueueImpl may be null for tests.
    GetTaskQueueImpl()->SetOnTaskStartedHandler(base::Bind(
        &MainThreadTaskQueue::OnTaskStarted, base::Unretained(this)));
    GetTaskQueueImpl()->SetOnTaskCompletedHandler(base::Bind(
        &MainThreadTaskQueue::OnTaskCompleted, base::Unretained(this)));
  }
}

MainThreadTaskQueue::~MainThreadTaskQueue() {}

void MainThreadTaskQueue::OnTaskStarted(const TaskQueue::Task& task,
                                        base::TimeTicks start) {
  if (renderer_scheduler_)
    renderer_scheduler_->OnTaskStarted(this, task, start);
}

void MainThreadTaskQueue::OnTaskCompleted(const TaskQueue::Task& task,
                                          base::TimeTicks start,
                                          base::TimeTicks end) {
  if (renderer_scheduler_)
    renderer_scheduler_->OnTaskCompleted(this, task, start, end);
}

void MainThreadTaskQueue::UnregisterTaskQueue() {
  if (renderer_scheduler_) {
    // RendererScheduler can be null in tests.
    renderer_scheduler_->OnUnregisterTaskQueue(this);
  }
  TaskQueue::UnregisterTaskQueue();
}

WebFrameScheduler* MainThreadTaskQueue::GetFrameScheduler() const {
  return web_frame_scheduler_;
}

void MainThreadTaskQueue::SetFrameScheduler(WebFrameScheduler* frame) {
  web_frame_scheduler_ = frame;
}

}  // namespace scheduler
}  // namespace blink
