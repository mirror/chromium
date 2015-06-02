// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/scheduler/renderer/renderer_scheduler_impl.h"

#include "base/bind.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/output/begin_frame_args.h"
#include "components/scheduler/child/nestable_single_thread_task_runner.h"
#include "components/scheduler/child/prioritizing_task_queue_selector.h"

namespace scheduler {

RendererSchedulerImpl::RendererSchedulerImpl(
    scoped_refptr<NestableSingleThreadTaskRunner> main_task_runner)
    : helper_(main_task_runner,
              "renderer.scheduler",
              TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
              TASK_QUEUE_COUNT),
      idle_helper_(&helper_,
                   this,
                   IDLE_TASK_QUEUE,
                   "renderer.scheduler",
                   TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                   "RendererSchedulerIdlePeriod",
                   base::TimeDelta()),
      control_task_runner_(helper_.ControlTaskRunner()),
      compositor_task_runner_(
          helper_.TaskRunnerForQueue(COMPOSITOR_TASK_QUEUE)),
      loading_task_runner_(helper_.TaskRunnerForQueue(LOADING_TASK_QUEUE)),
      timer_task_runner_(helper_.TaskRunnerForQueue(TIMER_TASK_QUEUE)),
      delayed_update_policy_runner_(
          base::Bind(&RendererSchedulerImpl::UpdatePolicy,
                     base::Unretained(this)),
          helper_.ControlTaskRunner()),
      current_policy_(Policy::NORMAL),
      renderer_hidden_(false),
      was_shutdown_(false),
      last_input_type_(blink::WebInputEvent::Undefined),
      input_stream_state_(InputStreamState::INACTIVE),
      policy_may_need_update_(&incoming_signals_lock_),
      timer_queue_suspend_count_(0),
      weak_factory_(this) {
  update_policy_closure_ = base::Bind(&RendererSchedulerImpl::UpdatePolicy,
                                      weak_factory_.GetWeakPtr());
  end_renderer_hidden_idle_period_closure_.Reset(base::Bind(
      &RendererSchedulerImpl::EndIdlePeriod, weak_factory_.GetWeakPtr()));

  for (size_t i = SchedulerHelper::TASK_QUEUE_COUNT; i < TASK_QUEUE_COUNT;
       i++) {
    helper_.SetQueueName(i, TaskQueueIdToString(static_cast<QueueId>(i)));
  }
  TRACE_EVENT_OBJECT_CREATED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this);
}

RendererSchedulerImpl::~RendererSchedulerImpl() {
  TRACE_EVENT_OBJECT_DELETED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this);
  // Ensure the renderer scheduler was shut down explicitly, because otherwise
  // we could end up having stale pointers to the Blink heap which has been
  // terminated by this point.
  DCHECK(was_shutdown_);
}

void RendererSchedulerImpl::Shutdown() {
  helper_.Shutdown();
  was_shutdown_ = true;
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::DefaultTaskRunner() {
  return helper_.DefaultTaskRunner();
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::CompositorTaskRunner() {
  helper_.CheckOnValidThread();
  return compositor_task_runner_;
}

scoped_refptr<SingleThreadIdleTaskRunner>
RendererSchedulerImpl::IdleTaskRunner() {
  return idle_helper_.IdleTaskRunner();
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::LoadingTaskRunner() {
  helper_.CheckOnValidThread();
  return loading_task_runner_;
}

scoped_refptr<base::SingleThreadTaskRunner>
RendererSchedulerImpl::TimerTaskRunner() {
  helper_.CheckOnValidThread();
  return timer_task_runner_;
}

bool RendererSchedulerImpl::CanExceedIdleDeadlineIfRequired() const {
  return idle_helper_.CanExceedIdleDeadlineIfRequired();
}

void RendererSchedulerImpl::AddTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  helper_.AddTaskObserver(task_observer);
}

void RendererSchedulerImpl::RemoveTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  helper_.RemoveTaskObserver(task_observer);
}

void RendererSchedulerImpl::WillBeginFrame(const cc::BeginFrameArgs& args) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::WillBeginFrame", "args", args.AsValue());
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  EndIdlePeriod();
  estimated_next_frame_begin_ = args.frame_time + args.interval;
  // TODO(skyostil): Wire up real notification of input events processing
  // instead of this approximation.
  DidProcessInputEvent(args.frame_time);
}

void RendererSchedulerImpl::DidCommitFrameToCompositor() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::DidCommitFrameToCompositor");
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  base::TimeTicks now(helper_.Now());
  if (now < estimated_next_frame_begin_) {
    // TODO(rmcilroy): Consider reducing the idle period based on the runtime of
    // the next pending delayed tasks (as currently done in for long idle times)
    idle_helper_.StartIdlePeriod(
        IdleHelper::IdlePeriodState::IN_SHORT_IDLE_PERIOD, now,
        estimated_next_frame_begin_);
  }
}

void RendererSchedulerImpl::BeginFrameNotExpectedSoon() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::BeginFrameNotExpectedSoon");
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return;

  // TODO(skyostil): Wire up real notification of input events processing
  // instead of this approximation.
  DidProcessInputEvent(base::TimeTicks());

  idle_helper_.EnableLongIdlePeriod();
}

void RendererSchedulerImpl::OnRendererHidden() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::OnRendererHidden");
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown() || renderer_hidden_)
    return;

  idle_helper_.EnableLongIdlePeriod();

  // Ensure that we stop running idle tasks after a few seconds of being hidden.
  end_renderer_hidden_idle_period_closure_.Cancel();
  base::TimeDelta end_idle_when_hidden_delay =
      base::TimeDelta::FromMilliseconds(kEndIdleWhenHiddenDelayMillis);
  control_task_runner_->PostDelayedTask(
      FROM_HERE, end_renderer_hidden_idle_period_closure_.callback(),
      end_idle_when_hidden_delay);
  renderer_hidden_ = true;

  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this, AsValue(helper_.Now()));
}

void RendererSchedulerImpl::OnRendererVisible() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::OnRendererVisible");
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown() || !renderer_hidden_)
    return;

  end_renderer_hidden_idle_period_closure_.Cancel();
  renderer_hidden_ = false;
  EndIdlePeriod();

  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this, AsValue(helper_.Now()));
}

void RendererSchedulerImpl::EndIdlePeriod() {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::EndIdlePeriod");
  helper_.CheckOnValidThread();
  idle_helper_.EndIdlePeriod();
}

void RendererSchedulerImpl::DidReceiveInputEventOnCompositorThread(
    const blink::WebInputEvent& web_input_event) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
               "RendererSchedulerImpl::DidReceiveInputEventOnCompositorThread");
  // We regard MouseMove events with the left mouse button down as a signal
  // that the user is doing something requiring a smooth frame rate.
  if (web_input_event.type == blink::WebInputEvent::MouseMove &&
      (web_input_event.modifiers & blink::WebInputEvent::LeftButtonDown)) {
    UpdateForInputEvent(web_input_event.type);
    return;
  }
  // Ignore all other mouse events because they probably don't signal user
  // interaction needing a smooth framerate. NOTE isMouseEventType returns false
  // for mouse wheel events, hence we regard them as user input.
  // Ignore keyboard events because it doesn't really make sense to enter
  // compositor priority for them.
  if (blink::WebInputEvent::isMouseEventType(web_input_event.type) ||
      blink::WebInputEvent::isKeyboardEventType(web_input_event.type)) {
    return;
  }
  UpdateForInputEvent(web_input_event.type);
}

void RendererSchedulerImpl::DidAnimateForInputOnCompositorThread() {
  UpdateForInputEvent(blink::WebInputEvent::Undefined);
}

void RendererSchedulerImpl::UpdateForInputEvent(
    blink::WebInputEvent::Type type) {
  base::AutoLock lock(incoming_signals_lock_);

  InputStreamState new_input_stream_state =
      ComputeNewInputStreamState(input_stream_state_, type, last_input_type_);

  if (input_stream_state_ != new_input_stream_state) {
    // Update scheduler policy if we should start a new policy mode.
    input_stream_state_ = new_input_stream_state;
    EnsureUrgentPolicyUpdatePostedOnMainThread(FROM_HERE);
  }
  last_input_receipt_time_on_compositor_ = helper_.Now();
  // Clear the last known input processing time so that we know an input event
  // is still queued up. This timestamp will be updated the next time the
  // compositor commits or becomes quiescent. Note that this function is always
  // called before the input event is processed either on the compositor or
  // main threads.
  last_input_process_time_on_main_ = base::TimeTicks();
  last_input_type_ = type;
}

void RendererSchedulerImpl::DidProcessInputEvent(
    base::TimeTicks begin_frame_time) {
  helper_.CheckOnValidThread();
  base::AutoLock lock(incoming_signals_lock_);
  if (input_stream_state_ == InputStreamState::INACTIVE)
    return;
  // Avoid marking input that arrived after the BeginFrame as processed.
  if (!begin_frame_time.is_null() &&
      begin_frame_time < last_input_receipt_time_on_compositor_)
    return;
  last_input_process_time_on_main_ = helper_.Now();
  UpdatePolicyLocked(UpdateType::MAY_EARLY_OUT_IF_POLICY_UNCHANGED);
}

bool RendererSchedulerImpl::IsHighPriorityWorkAnticipated() {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return false;

  MaybeUpdatePolicy();
  // The touchstart and compositor policies indicate a strong likelihood of
  // high-priority work in the near future.
  return SchedulerPolicy() == Policy::COMPOSITOR_PRIORITY ||
         SchedulerPolicy() == Policy::TOUCHSTART_PRIORITY;
}

bool RendererSchedulerImpl::ShouldYieldForHighPriorityWork() {
  helper_.CheckOnValidThread();
  if (helper_.IsShutdown())
    return false;

  MaybeUpdatePolicy();
  // We only yield if we are in the compositor priority and there is compositor
  // work outstanding, or if we are in the touchstart response priority.
  // Note: even though the control queue is higher priority we don't yield for
  // it since these tasks are not user-provided work and they are only intended
  // to run before the next task, not interrupt the tasks.
  switch (SchedulerPolicy()) {
    case Policy::NORMAL:
      return false;

    case Policy::COMPOSITOR_PRIORITY:
      return !helper_.IsQueueEmpty(COMPOSITOR_TASK_QUEUE);

    case Policy::TOUCHSTART_PRIORITY:
      return true;

    default:
      NOTREACHED();
      return false;
  }
}

base::TimeTicks RendererSchedulerImpl::CurrentIdleTaskDeadlineForTesting()
    const {
  return idle_helper_.CurrentIdleTaskDeadline();
}

RendererSchedulerImpl::Policy RendererSchedulerImpl::SchedulerPolicy() const {
  helper_.CheckOnValidThread();
  return current_policy_;
}

void RendererSchedulerImpl::MaybeUpdatePolicy() {
  helper_.CheckOnValidThread();
  if (policy_may_need_update_.IsSet()) {
    UpdatePolicy();
  }
}

void RendererSchedulerImpl::EnsureUrgentPolicyUpdatePostedOnMainThread(
    const tracked_objects::Location& from_here) {
  // TODO(scheduler-dev): Check that this method isn't called from the main
  // thread.
  incoming_signals_lock_.AssertAcquired();
  if (!policy_may_need_update_.IsSet()) {
    policy_may_need_update_.SetWhileLocked(true);
    control_task_runner_->PostTask(from_here, update_policy_closure_);
  }
}

void RendererSchedulerImpl::UpdatePolicy() {
  base::AutoLock lock(incoming_signals_lock_);
  UpdatePolicyLocked(UpdateType::MAY_EARLY_OUT_IF_POLICY_UNCHANGED);
}

void RendererSchedulerImpl::ForceUpdatePolicy() {
  base::AutoLock lock(incoming_signals_lock_);
  UpdatePolicyLocked(UpdateType::FORCE_UPDATE);
}

void RendererSchedulerImpl::UpdatePolicyLocked(UpdateType update_type) {
  helper_.CheckOnValidThread();
  incoming_signals_lock_.AssertAcquired();
  if (helper_.IsShutdown())
    return;

  base::TimeTicks now = helper_.Now();
  policy_may_need_update_.SetWhileLocked(false);

  base::TimeDelta new_policy_duration;
  Policy new_policy = ComputeNewPolicy(now, &new_policy_duration);
  if (new_policy_duration > base::TimeDelta()) {
    current_policy_expiration_time_ = now + new_policy_duration;
    delayed_update_policy_runner_.SetDeadline(FROM_HERE, new_policy_duration,
                                              now);
  } else {
    current_policy_expiration_time_ = base::TimeTicks();
  }

  if (update_type == UpdateType::MAY_EARLY_OUT_IF_POLICY_UNCHANGED &&
      new_policy == current_policy_)
    return;

  bool policy_disables_timers = false;

  switch (new_policy) {
    case Policy::COMPOSITOR_PRIORITY:
      helper_.SetQueuePriority(COMPOSITOR_TASK_QUEUE,
                               PrioritizingTaskQueueSelector::HIGH_PRIORITY);
      // TODO(scheduler-dev): Add a task priority between HIGH and BEST_EFFORT
      // that still has some guarantee of running.
      helper_.SetQueuePriority(
          LOADING_TASK_QUEUE,
          PrioritizingTaskQueueSelector::BEST_EFFORT_PRIORITY);
      break;
    case Policy::TOUCHSTART_PRIORITY:
      helper_.SetQueuePriority(COMPOSITOR_TASK_QUEUE,
                               PrioritizingTaskQueueSelector::HIGH_PRIORITY);
      helper_.DisableQueue(LOADING_TASK_QUEUE);
      // TODO(alexclarke): Set policy_disables_timers once the blink TimerBase
      // refactor is safely landed.
      break;
    case Policy::NORMAL:
      helper_.SetQueuePriority(COMPOSITOR_TASK_QUEUE,
                               PrioritizingTaskQueueSelector::NORMAL_PRIORITY);
      helper_.SetQueuePriority(LOADING_TASK_QUEUE,
                               PrioritizingTaskQueueSelector::NORMAL_PRIORITY);
      break;
    default:
      NOTREACHED();
  }
  if (timer_queue_suspend_count_ != 0 || policy_disables_timers) {
    helper_.DisableQueue(TIMER_TASK_QUEUE);
  } else {
    helper_.SetQueuePriority(TIMER_TASK_QUEUE,
                             PrioritizingTaskQueueSelector::NORMAL_PRIORITY);
  }
  DCHECK(helper_.IsQueueEnabled(COMPOSITOR_TASK_QUEUE));
  if (new_policy != Policy::TOUCHSTART_PRIORITY)
    DCHECK(helper_.IsQueueEnabled(LOADING_TASK_QUEUE));

  current_policy_ = new_policy;

  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"), "RendererScheduler",
      this, AsValueLocked(now));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("renderer.scheduler"),
                 "RendererScheduler.policy", current_policy_);
}

RendererSchedulerImpl::Policy RendererSchedulerImpl::ComputeNewPolicy(
    base::TimeTicks now,
    base::TimeDelta* new_policy_duration) {
  helper_.CheckOnValidThread();
  incoming_signals_lock_.AssertAcquired();

  Policy new_policy = Policy::NORMAL;
  *new_policy_duration = base::TimeDelta();

  if (input_stream_state_ == InputStreamState::INACTIVE)
    return new_policy;

  Policy input_priority_policy =
      input_stream_state_ ==
              InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE
          ? Policy::TOUCHSTART_PRIORITY
          : Policy::COMPOSITOR_PRIORITY;
  base::TimeDelta time_left_in_policy = TimeLeftInInputEscalatedPolicy(now);
  if (time_left_in_policy > base::TimeDelta()) {
    new_policy = input_priority_policy;
    *new_policy_duration = time_left_in_policy;
  } else {
    // Reset |input_stream_state_| to ensure
    // DidReceiveInputEventOnCompositorThread will post an UpdatePolicy task
    // when it's next called.
    input_stream_state_ = InputStreamState::INACTIVE;
  }
  return new_policy;
}

base::TimeDelta RendererSchedulerImpl::TimeLeftInInputEscalatedPolicy(
    base::TimeTicks now) const {
  helper_.CheckOnValidThread();
  // TODO(rmcilroy): Change this to DCHECK_EQ when crbug.com/463869 is fixed.
  DCHECK(input_stream_state_ != InputStreamState::INACTIVE);
  incoming_signals_lock_.AssertAcquired();

  base::TimeDelta escalated_priority_duration =
      base::TimeDelta::FromMilliseconds(kPriorityEscalationAfterInputMillis);
  base::TimeDelta time_left_in_policy;
  if (last_input_process_time_on_main_.is_null() &&
      !helper_.IsQueueEmpty(COMPOSITOR_TASK_QUEUE)) {
    // If the input event is still pending, go into input prioritized policy
    // and check again later.
    time_left_in_policy = escalated_priority_duration;
  } else {
    // Otherwise make sure the input prioritization policy ends on time.
    base::TimeTicks new_priority_end(
        std::max(last_input_receipt_time_on_compositor_,
                 last_input_process_time_on_main_) +
        escalated_priority_duration);
    time_left_in_policy = new_priority_end - now;
  }
  return time_left_in_policy;
}

bool RendererSchedulerImpl::CanEnterLongIdlePeriod(
    base::TimeTicks now,
    base::TimeDelta* next_long_idle_period_delay_out) {
  helper_.CheckOnValidThread();

  MaybeUpdatePolicy();
  if (SchedulerPolicy() == Policy::TOUCHSTART_PRIORITY) {
    // Don't start a long idle task in touch start priority, try again when
    // the policy is scheduled to end.
    *next_long_idle_period_delay_out = current_policy_expiration_time_ - now;
    return false;
  }
  return true;
}

SchedulerHelper* RendererSchedulerImpl::GetSchedulerHelperForTesting() {
  return &helper_;
}

void RendererSchedulerImpl::SuspendTimerQueue() {
  helper_.CheckOnValidThread();
  timer_queue_suspend_count_++;
  ForceUpdatePolicy();
  DCHECK(!helper_.IsQueueEnabled(TIMER_TASK_QUEUE));
}

void RendererSchedulerImpl::ResumeTimerQueue() {
  helper_.CheckOnValidThread();
  timer_queue_suspend_count_--;
  DCHECK_GE(timer_queue_suspend_count_, 0);
  ForceUpdatePolicy();
}

// static
const char* RendererSchedulerImpl::TaskQueueIdToString(QueueId queue_id) {
  switch (queue_id) {
    case IDLE_TASK_QUEUE:
      return "idle_tq";
    case COMPOSITOR_TASK_QUEUE:
      return "compositor_tq";
    case LOADING_TASK_QUEUE:
      return "loading_tq";
    case TIMER_TASK_QUEUE:
      return "timer_tq";
    default:
      return SchedulerHelper::TaskQueueIdToString(
          static_cast<SchedulerHelper::QueueId>(queue_id));
  }
}

// static
const char* RendererSchedulerImpl::PolicyToString(Policy policy) {
  switch (policy) {
    case Policy::NORMAL:
      return "normal";
    case Policy::COMPOSITOR_PRIORITY:
      return "compositor";
    case Policy::TOUCHSTART_PRIORITY:
      return "touchstart";
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* RendererSchedulerImpl::InputStreamStateToString(
    InputStreamState state) {
  switch (state) {
    case InputStreamState::INACTIVE:
      return "inactive";
    case InputStreamState::ACTIVE:
      return "active";
    case InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE:
      return "active_and_awaiting_touchstart_response";
    default:
      NOTREACHED();
      return nullptr;
  }
}

scoped_refptr<base::trace_event::ConvertableToTraceFormat>
RendererSchedulerImpl::AsValue(base::TimeTicks optional_now) const {
  base::AutoLock lock(incoming_signals_lock_);
  return AsValueLocked(optional_now);
}

scoped_refptr<base::trace_event::ConvertableToTraceFormat>
RendererSchedulerImpl::AsValueLocked(base::TimeTicks optional_now) const {
  helper_.CheckOnValidThread();
  incoming_signals_lock_.AssertAcquired();

  if (optional_now.is_null())
    optional_now = helper_.Now();
  scoped_refptr<base::trace_event::TracedValue> state =
      new base::trace_event::TracedValue();

  state->SetString("current_policy", PolicyToString(current_policy_));
  state->SetString("idle_period_state",
                   IdleHelper::IdlePeriodStateToString(
                       idle_helper_.SchedulerIdlePeriodState()));
  state->SetBoolean("renderer_hidden_", renderer_hidden_);
  state->SetString("input_stream_state",
                   InputStreamStateToString(input_stream_state_));
  state->SetDouble("now", (optional_now - base::TimeTicks()).InMillisecondsF());
  state->SetDouble("last_input_receipt_time_on_compositor_",
                   (last_input_receipt_time_on_compositor_ - base::TimeTicks())
                       .InMillisecondsF());
  state->SetDouble(
      "last_input_process_time_on_main_",
      (last_input_process_time_on_main_ - base::TimeTicks()).InMillisecondsF());
  state->SetDouble(
      "estimated_next_frame_begin",
      (estimated_next_frame_begin_ - base::TimeTicks()).InMillisecondsF());

  return state;
}

RendererSchedulerImpl::InputStreamState
RendererSchedulerImpl::ComputeNewInputStreamState(
    InputStreamState current_state,
    blink::WebInputEvent::Type new_input_type,
    blink::WebInputEvent::Type last_input_type) {
  switch (new_input_type) {
    case blink::WebInputEvent::TouchStart:
      return InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE;

    case blink::WebInputEvent::TouchMove:
      // Observation of consecutive touchmoves is a strong signal that the page
      // is consuming the touch sequence, in which case touchstart response
      // prioritization is no longer necessary. Otherwise, the initial touchmove
      // should preserve the touchstart response pending state.
      if (current_state ==
          InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE) {
        return last_input_type == blink::WebInputEvent::TouchMove
                   ? InputStreamState::ACTIVE
                   : InputStreamState::ACTIVE_AND_AWAITING_TOUCHSTART_RESPONSE;
      }
      break;

    case blink::WebInputEvent::GestureTapDown:
    case blink::WebInputEvent::GestureShowPress:
    case blink::WebInputEvent::GestureFlingCancel:
    case blink::WebInputEvent::GestureScrollEnd:
      // With no observable effect, these meta events do not indicate a
      // meaningful touchstart response and should not impact task priority.
      return current_state;

    default:
      break;
  }
  return InputStreamState::ACTIVE;
}

}  // namespace scheduler
