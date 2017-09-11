// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/incoming_task_queue.h"

#include <limits>
#include <utility>

#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "build/build_config.h"

namespace base {
namespace internal {

namespace {

#if DCHECK_IS_ON()
// Delays larger than this are often bogus, and a warning should be emitted in
// debug builds to warn developers.  http://crbug.com/450045
const int kTaskDelayWarningThresholdInSeconds =
    14 * 24 * 60 * 60;  // 14 days.
#endif

// Returns true if MessagePump::ScheduleWork() must be called one
// time for every task that is added to the MessageLoop incoming queue.
bool AlwaysNotifyPump(MessageLoop::Type type) {
#if defined(OS_ANDROID)
  // The Android UI message loop needs to get notified each time a task is
  // added
  // to the incoming queue.
  return type == MessageLoop::TYPE_UI || type == MessageLoop::TYPE_JAVA;
#else
  return false;
#endif
}

TimeTicks CalculateDelayedRuntime(TimeDelta delay) {
  TimeTicks delayed_run_time;
  if (delay > TimeDelta())
    delayed_run_time = TimeTicks::Now() + delay;
  else
    DCHECK_EQ(delay.InMilliseconds(), 0) << "delay should not be negative";
  return delayed_run_time;
}

}  // namespace

IncomingTaskQueue::IncomingTaskQueue(MessageLoop* message_loop)
    : always_schedule_work_(AlwaysNotifyPump(message_loop->type())),
      message_loop_(message_loop)
#if DCHECK_IS_ON()
      ,
      post_pending_task_event_(WaitableEvent::ResetPolicy::MANUAL,
                               WaitableEvent::InitialState::SIGNALED)
#endif
{
  DCHECK(message_loop_);
}

bool IncomingTaskQueue::AddToIncomingQueue(
    const tracked_objects::Location& from_here,
    OnceClosure task,
    TimeDelta delay,
    bool nestable) {
  // Use CHECK instead of DCHECK to crash earlier. See http://crbug.com/711167
  // for details.
  CHECK(task);
  DLOG_IF(WARNING,
          delay.InSeconds() > kTaskDelayWarningThresholdInSeconds)
      << "Requesting super-long task delay period of " << delay.InSeconds()
      << " seconds from here: " << from_here.ToString();

  PendingTask pending_task(from_here, std::move(task),
                           CalculateDelayedRuntime(delay), nestable);
#if defined(OS_WIN)
  // We consider the task needs a high resolution timer if the delay is
  // more than 0 and less than 32ms. This caps the relative error to
  // less than 50% : a 33ms wait can wake at 48ms since the default
  // resolution on Windows is between 10 and 15ms.
  if (delay > TimeDelta() &&
      delay.InMilliseconds() < (2 * Time::kMinLowResolutionThresholdMs)) {
    pending_task.is_high_res = true;
  }
#endif
  return PostPendingTask(&pending_task);
}

bool IncomingTaskQueue::IsIdleForTesting() {
  AutoLock lock(incoming_queue_lock_);
  return incoming_queue_.empty();
}

int IncomingTaskQueue::ReloadWorkQueue(TaskQueue* work_queue) {
  // Make sure no tasks are lost.
  DCHECK(work_queue->empty());

  // Acquire all we can from the inter-thread queue with one lock acquisition.
  AutoLock lock(incoming_queue_lock_);
  if (incoming_queue_.empty()) {
    // If the loop attempts to reload but there are no tasks in the incoming
    // queue, that means it will go to sleep waiting for more work. If the
    // incoming queue becomes nonempty we need to schedule it again.
    message_loop_scheduled_ = false;
  } else {
    incoming_queue_.swap(*work_queue);
  }
  // Reset the count of high resolution tasks since our queue is now empty.
  int high_res_tasks = high_res_task_count_;
  high_res_task_count_ = 0;
  return high_res_tasks;
}

void IncomingTaskQueue::WillDestroyCurrentMessageLoop() {
  post_pending_task_tracker_.StopAcceptingPostTaskCalls();
  message_loop_ = nullptr;
}

void IncomingTaskQueue::StartScheduling() {
  bool schedule_work;
  {
    AutoLock lock(incoming_queue_lock_);
    DCHECK(!is_ready_for_scheduling_);
    DCHECK(!message_loop_scheduled_);
    is_ready_for_scheduling_ = true;
    schedule_work = !incoming_queue_.empty();
  }
  if (schedule_work) {
    DCHECK(message_loop_);
    // Don't need to lock |message_loop_lock_| here because this function is
    // called by MessageLoop on its thread.
    message_loop_->ScheduleWork();
  }
}

IncomingTaskQueue::PostPendingTaskTracker::ScopedCall::ScopedCall(
    PostPendingTaskTracker* post_pending_task_tracker)
    : post_pending_task_tracker_(post_pending_task_tracker),
      should_continue_(
          post_pending_task_tracker_->IncrementPostPendingTasksInFlight()) {
  DCHECK(post_pending_task_tracker_);
}

IncomingTaskQueue::PostPendingTaskTracker::ScopedCall::~ScopedCall() {
  if (should_continue_)
    post_pending_task_tracker_->DecrementPostPendingTasksInFlight();
}

IncomingTaskQueue::PostPendingTaskTracker::PostPendingTaskTracker() = default;
IncomingTaskQueue::PostPendingTaskTracker::~PostPendingTaskTracker() = default;

void IncomingTaskQueue::PostPendingTaskTracker::StopAcceptingPostTaskCalls() {
  DCHECK(!shutdown_event_);
  shutdown_event_ = std::make_unique<WaitableEvent>(
      WaitableEvent::ResetPolicy::MANUAL,
      WaitableEvent::InitialState::NOT_SIGNALED);
  const auto new_bits =
      subtle::Barrier_AtomicIncrement(&state_, kShutdownStartedMask);
  DCHECK(new_bits & kShutdownStartedMask);

  const auto num_tasks_blocking_shutdown =
      new_bits >> kNumPostTasksInFlightBitOffset;
  if (num_tasks_blocking_shutdown == 0)
    shutdown_event_->Signal();

  {
    base::ThreadRestrictions::ScopedAllowWait allow_wait;
    shutdown_event_->Wait();
  }
}

bool IncomingTaskQueue::PostPendingTaskTracker::
    IncrementPostPendingTasksInFlight() {
#if DCHECK_IS_ON()
  const auto num_post_tasks_in_flight =
      subtle::NoBarrier_Load(&state_) >> kNumPostTasksInFlightBitOffset;
  DCHECK_LT(num_post_tasks_in_flight,
            std::numeric_limits<subtle::Atomic32>::max() -
                kNumPostTasksInFlightIncrement);
#endif
  const auto new_bits = subtle::NoBarrier_AtomicIncrement(
      &state_, kNumPostTasksInFlightIncrement);
  bool shutdown_started = new_bits & kShutdownStartedMask;
  if (shutdown_started) {
    subtle::NoBarrier_AtomicIncrement(&state_, -kNumPostTasksInFlightIncrement);
    return false;
  }
  return true;
}

void IncomingTaskQueue::PostPendingTaskTracker::
    DecrementPostPendingTasksInFlight() {
  const auto new_bits = subtle::NoBarrier_AtomicIncrement(
      &state_, -kNumPostTasksInFlightIncrement);
  if (new_bits & kShutdownStartedMask) {
    const auto num_tasks_blocking_shutdown =
        new_bits >> kNumPostTasksInFlightBitOffset;
    if (num_tasks_blocking_shutdown == 0) {
      // Need to make sure this thread has access to |shutdown_event_| created
      // on a different thread.
      subtle::MemoryBarrier();
      DCHECK(shutdown_event_);
      DCHECK(!shutdown_event_->IsSignaled());
      shutdown_event_->Signal();
    }
  }
}

bool IncomingTaskQueue::PostPendingTaskTracker::
    IsPostPendingTaskInFlightForTesting() const {
  const auto num_post_tasks_in_flight =
      subtle::NoBarrier_Load(&state_) >> kNumPostTasksInFlightBitOffset;
  return num_post_tasks_in_flight > 0;
}

IncomingTaskQueue::~IncomingTaskQueue() {
  // Verify that WillDestroyCurrentMessageLoop() has been called.
  DCHECK(!message_loop_);
}

void IncomingTaskQueue::RunTask(PendingTask* pending_task) {
  SEQUENCE_CHECKER(sequence_checker_);
  task_annotator_.RunTask("MessageLoop::PostTask", pending_task);
}

void IncomingTaskQueue::PausePostPendingTaskForTesting() {
#if DCHECK_IS_ON()
  post_pending_task_event_.Reset();
#endif
}

void IncomingTaskQueue::ResumePostPendingTaskForTesting() {
#if DCHECK_IS_ON()
  post_pending_task_event_.Signal();
#endif
}

bool IncomingTaskQueue::IsPostPendingTaskInFlightForTesting() {
  return post_pending_task_tracker_.IsPostPendingTaskInFlightForTesting();
}

bool IncomingTaskQueue::PostPendingTask(PendingTask* pending_task) {
  // Warning: Don't try to short-circuit, and handle this thread's tasks more
  // directly, as it could starve handling of foreign threads.  Put every task
  // into this queue.
  PostPendingTaskTracker::ScopedCall scoped_post_task_call(
      &post_pending_task_tracker_);
  if (!scoped_post_task_call.should_continue()) {
    // Clear the pending task outside of |incoming_queue_lock_| to prevent any
    // chance of self-deadlock if destroying a task also posts a task to this
    // queue.
    pending_task->task.Reset();
    return false;
  }

  bool schedule_work = false;
  {
    AutoLock auto_lock(incoming_queue_lock_);
    schedule_work = PostPendingTaskLockRequired(pending_task);
  }

#if DCHECK_IS_ON()
  {
    base::ThreadRestrictions::ScopedAllowWait allow_wait;
    post_pending_task_event_.Wait();
  }
#endif

  // Wake up the message loop and schedule work. This is done outside
  // |incoming_queue_lock_| because signaling the message loop may cause this
  // thread to be switched. If |incoming_queue_lock_| is held, any other thread
  // that wants to post a task will be blocked until this thread switches back
  // in and releases |incoming_queue_lock_|.
  if (schedule_work) {
    DCHECK(message_loop_);
    message_loop_->ScheduleWork();
  }

  return true;
}

bool IncomingTaskQueue::PostPendingTaskLockRequired(PendingTask* pending_task) {
  incoming_queue_lock_.AssertAcquired();

#if defined(OS_WIN)
  if (pending_task->is_high_res)
    ++high_res_task_count_;
#endif

  // Initialize the sequence number. The sequence number is used for delayed
  // tasks (to facilitate FIFO sorting when two tasks have the same
  // delayed_run_time value) and for identifying the task in about:tracing.
  pending_task->sequence_num = next_sequence_num_++;

  task_annotator_.DidQueueTask("MessageLoop::PostTask", *pending_task);

  bool was_empty = incoming_queue_.empty();
  incoming_queue_.push(std::move(*pending_task));

  if (is_ready_for_scheduling_ &&
      (always_schedule_work_ || (!message_loop_scheduled_ && was_empty))) {
    // After we've scheduled the message loop, we do not need to do so again
    // until we know it has processed all of the work in our queue and is
    // waiting for more work again. The message loop will always attempt to
    // reload from the incoming queue before waiting again so we clear this
    // flag in ReloadWorkQueue().
    message_loop_scheduled_ = true;
    return true;
  }
  return false;
}

}  // namespace internal
}  // namespace base
