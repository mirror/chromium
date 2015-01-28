// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/scheduler/renderer_scheduler_impl.h"

#include "base/bind.h"
#include "base/debug/trace_event.h"
#include "base/debug/trace_event_argument.h"
#include "base/message_loop/message_loop_proxy.h"
#include "cc/output/begin_frame_args.h"
#include "content/renderer/scheduler/renderer_task_queue_selector.h"
#include "ui/gfx/frame_time.h"

namespace content {

RendererSchedulerImpl::RendererSchedulerImpl(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner)
    : renderer_task_queue_selector_(new RendererTaskQueueSelector()),
      task_queue_manager_(
          new TaskQueueManager(TASK_QUEUE_COUNT,
                               main_task_runner,
                               renderer_task_queue_selector_.get())),
      control_task_runner_(
          task_queue_manager_->TaskRunnerForQueue(CONTROL_TASK_QUEUE)),
      default_task_runner_(
          task_queue_manager_->TaskRunnerForQueue(DEFAULT_TASK_QUEUE)),
      compositor_task_runner_(
          task_queue_manager_->TaskRunnerForQueue(COMPOSITOR_TASK_QUEUE)),
      current_policy_(NORMAL_PRIORITY_POLICY),
      policy_may_need_update_(&incoming_signals_lock_),
      weak_factory_(this) {
  weak_renderer_scheduler_ptr_ = weak_factory_.GetWeakPtr();
  update_policy_closure_ = base::Bind(&RendererSchedulerImpl::UpdatePolicy,
                                      weak_renderer_scheduler_ptr_);
  end_idle_period_closure_.Reset(base::Bind(
      &RendererSchedulerImpl::EndIdlePeriod, weak_renderer_scheduler_ptr_));
  idle_task_runner_ = make_scoped_refptr(new SingleThreadIdleTaskRunner(
      task_queue_manager_->TaskRunnerForQueue(IDLE_TASK_QUEUE),
      base::Bind(&RendererSchedulerImpl::CurrentIdleTaskDeadlineCallback,
                 weak_renderer_scheduler_ptr_)));
  renderer_task_queue_selector_->SetQueuePriority(
      CONTROL_TASK_QUEUE, RendererTaskQueueSelector::CONTROL_PRIORITY);
  renderer_task_queue_selector_->DisableQueue(IDLE_TASK_QUEUE);
  task_queue_manager_->SetAutoPump(IDLE_TASK_QUEUE, false);

  for (size_t i = 0; i < TASK_QUEUE_COUNT; i++) {
    task_queue_manager_->SetQueueName(
        i, TaskQueueIdToString(static_cast<QueueId>(i)));
  }
  TRACE_EVENT_OBJECT_CREATED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this);
}

RendererSchedulerImpl::~RendererSchedulerImpl() {
  TRACE_EVENT_OBJECT_DELETED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this);
}

void RendererSchedulerImpl::Shutdown() {
  main_thread_checker_.CalledOnValidThread();
  task_queue_manager_.reset();
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::DefaultTaskRunner() {
  main_thread_checker_.CalledOnValidThread();
  return default_task_runner_;
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::CompositorTaskRunner() {
  main_thread_checker_.CalledOnValidThread();
  return compositor_task_runner_;
}

scoped_refptr<SingleThreadIdleTaskRunner>
RendererSchedulerImpl::IdleTaskRunner() {
  main_thread_checker_.CalledOnValidThread();
  return idle_task_runner_;
}

void RendererSchedulerImpl::WillBeginFrame(const cc::BeginFrameArgs& args) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::WillBeginFrame", "args", args.AsValue());
  main_thread_checker_.CalledOnValidThread();
  if (!task_queue_manager_)
    return;

  EndIdlePeriod();
  estimated_next_frame_begin_ = args.frame_time + args.interval;
}

void RendererSchedulerImpl::DidCommitFrameToCompositor() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::DidCommitFrameToCompositor");
  main_thread_checker_.CalledOnValidThread();
  if (!task_queue_manager_)
    return;

  base::TimeTicks now(Now());
  if (now < estimated_next_frame_begin_) {
    StartIdlePeriod();
    control_task_runner_->PostDelayedTask(FROM_HERE,
                                          end_idle_period_closure_.callback(),
                                          estimated_next_frame_begin_ - now);
  }
}

void RendererSchedulerImpl::DidReceiveInputEventOnCompositorThread(
    blink::WebInputEvent::Type type) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::DidReceiveInputEventOnCompositorThread");
  // Ignore mouse events because on windows these can very frequent.
  // Ignore keyboard events because it doesn't really make sense to enter
  // compositor priority for them.
  if (blink::WebInputEvent::isMouseEventType(type) ||
      blink::WebInputEvent::isKeyboardEventType(type)) {
    return;
  }
  UpdateForInputEvent();
}

void RendererSchedulerImpl::DidAnimateForInputOnCompositorThread() {
  UpdateForInputEvent();
}

void RendererSchedulerImpl::UpdateForInputEvent() {
  base::AutoLock lock(incoming_signals_lock_);
  if (last_input_time_.is_null()) {
    // Update scheduler policy if should start a new compositor policy mode.
    policy_may_need_update_.SetLocked(true);
    PostUpdatePolicyOnControlRunner(base::TimeDelta());
  }
  last_input_time_ = Now();
}

bool RendererSchedulerImpl::ShouldYieldForHighPriorityWork() {
  main_thread_checker_.CalledOnValidThread();
  if (!task_queue_manager_)
    return false;

  MaybeUpdatePolicy();
  // We only yield if we are in the compositor priority and there is compositor
  // work outstanding. Note: even though the control queue is higher priority
  // we don't yield for it since these tasks are not user-provided work and they
  // are only intended to run before the next task, not interrupt the tasks.
  return SchedulerPolicy() == COMPOSITOR_PRIORITY_POLICY &&
         !task_queue_manager_->IsQueueEmpty(COMPOSITOR_TASK_QUEUE);
}

void RendererSchedulerImpl::CurrentIdleTaskDeadlineCallback(
    base::TimeTicks* deadline_out) const {
  main_thread_checker_.CalledOnValidThread();
  *deadline_out = estimated_next_frame_begin_;
}

RendererSchedulerImpl::Policy RendererSchedulerImpl::SchedulerPolicy() const {
  main_thread_checker_.CalledOnValidThread();
  return current_policy_;
}

void RendererSchedulerImpl::MaybeUpdatePolicy() {
  main_thread_checker_.CalledOnValidThread();
  if (policy_may_need_update_.IsSet()) {
    UpdatePolicy();
  }
}

void RendererSchedulerImpl::PostUpdatePolicyOnControlRunner(
    base::TimeDelta delay) {
  control_task_runner_->PostDelayedTask(
      FROM_HERE, update_policy_closure_, delay);
}

void RendererSchedulerImpl::UpdatePolicy() {
  main_thread_checker_.CalledOnValidThread();
  if (!task_queue_manager_)
    return;

  base::AutoLock lock(incoming_signals_lock_);
  base::TimeTicks now;
  policy_may_need_update_.SetLocked(false);

  Policy new_policy = NORMAL_PRIORITY_POLICY;
  if (!last_input_time_.is_null()) {
    base::TimeDelta compositor_priority_duration =
        base::TimeDelta::FromMilliseconds(kCompositorPriorityAfterTouchMillis);
    base::TimeTicks compositor_priority_end(last_input_time_ +
                                            compositor_priority_duration);
    now = Now();
    if (compositor_priority_end > now) {
      PostUpdatePolicyOnControlRunner(compositor_priority_end - now);
      new_policy = COMPOSITOR_PRIORITY_POLICY;
    } else {
      // Null out last_input_time_ to ensure
      // DidReceiveInputEventOnCompositorThread will post an
      // UpdatePolicy task when it's next called.
      last_input_time_ = base::TimeTicks();
    }
  }

  if (new_policy == current_policy_)
    return;

  switch (new_policy) {
    case COMPOSITOR_PRIORITY_POLICY:
      renderer_task_queue_selector_->SetQueuePriority(
          COMPOSITOR_TASK_QUEUE, RendererTaskQueueSelector::HIGH_PRIORITY);
      break;
    case NORMAL_PRIORITY_POLICY:
      renderer_task_queue_selector_->SetQueuePriority(
          COMPOSITOR_TASK_QUEUE, RendererTaskQueueSelector::NORMAL_PRIORITY);
      break;
  }
  current_policy_ = new_policy;

  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this, AsValueLocked(now));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "RendererScheduler.policy", current_policy_);
}

void RendererSchedulerImpl::StartIdlePeriod() {
  TRACE_EVENT_ASYNC_BEGIN0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                           "RendererSchedulerIdlePeriod", this);
  main_thread_checker_.CalledOnValidThread();
  renderer_task_queue_selector_->EnableQueue(
      IDLE_TASK_QUEUE, RendererTaskQueueSelector::BEST_EFFORT_PRIORITY);
  task_queue_manager_->PumpQueue(IDLE_TASK_QUEUE);
}

void RendererSchedulerImpl::EndIdlePeriod() {
  TRACE_EVENT_ASYNC_END0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                         "RendererSchedulerIdlePeriod", this);
  main_thread_checker_.CalledOnValidThread();
  end_idle_period_closure_.Cancel();
  renderer_task_queue_selector_->DisableQueue(IDLE_TASK_QUEUE);
}

base::TimeTicks RendererSchedulerImpl::Now() const {
  return gfx::FrameTime::Now();
}

RendererSchedulerImpl::PollableNeedsUpdateFlag::PollableNeedsUpdateFlag(
    base::Lock* write_lock_)
    : flag_(false), write_lock_(write_lock_) {
}

RendererSchedulerImpl::PollableNeedsUpdateFlag::~PollableNeedsUpdateFlag() {
}

void RendererSchedulerImpl::PollableNeedsUpdateFlag::SetLocked(bool value) {
  write_lock_->AssertAcquired();
  base::subtle::Release_Store(&flag_, value);
}

bool RendererSchedulerImpl::PollableNeedsUpdateFlag::IsSet() const {
  thread_checker_.CalledOnValidThread();
  return base::subtle::Acquire_Load(&flag_) != 0;
}

// static
const char* RendererSchedulerImpl::TaskQueueIdToString(QueueId queue_id) {
  switch (queue_id) {
    case DEFAULT_TASK_QUEUE:
      return "default_tq";
    case COMPOSITOR_TASK_QUEUE:
      return "compositor_tq";
    case IDLE_TASK_QUEUE:
      return "idle_tq";
    case CONTROL_TASK_QUEUE:
      return "control_tq";
    default:
      NOTREACHED();
      return nullptr;
  }
}

// static
const char* RendererSchedulerImpl::PolicyToString(Policy policy) {
  switch (policy) {
    case NORMAL_PRIORITY_POLICY:
      return "normal";
    case COMPOSITOR_PRIORITY_POLICY:
      return "compositor";
    default:
      NOTREACHED();
      return nullptr;
  }
}

scoped_refptr<base::debug::ConvertableToTraceFormat>
RendererSchedulerImpl::AsValueLocked(base::TimeTicks optional_now) const {
  main_thread_checker_.CalledOnValidThread();
  incoming_signals_lock_.AssertAcquired();

  if (optional_now.is_null())
    optional_now = Now();
  scoped_refptr<base::debug::TracedValue> state =
      new base::debug::TracedValue();

  state->SetString("current_policy", PolicyToString(current_policy_));
  state->SetDouble("now", (optional_now - base::TimeTicks()).InMillisecondsF());
  state->SetDouble("last_input_time",
                   (last_input_time_ - base::TimeTicks()).InMillisecondsF());
  state->SetDouble(
      "estimated_next_frame_begin",
      (estimated_next_frame_begin_ - base::TimeTicks()).InMillisecondsF());

  return state;
}

}  // namespace content
