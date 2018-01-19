// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SCHEDULER_INCOMING_TASK_QUEUE_H_
#define BASE_TASK_SCHEDULER_SCHEDULER_INCOMING_TASK_QUEUE_H_

#include <memory>

#include "base/base_export.h"
#include "base/containers/circular_deque.h"
#include "base/debug/task_annotator.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/atomic_flag.h"
#include "base/task_scheduler/scheduler_incoming_task_queue_task.h"
#include "base/task_scheduler/scheduler_lock.h"
#include "base/task_scheduler/sequence.h"

namespace base {

class MessageLoop;

namespace internal {

class TaskTracker;

class BASE_EXPORT SchedulerIncomingTaskQueue
    : public RefCountedThreadSafe<SchedulerIncomingTaskQueue> {
 public:
  // Provides a read and remove only view into a task queue.
  class ReadAndRemoveOnlyQueue {
   public:
    ReadAndRemoveOnlyQueue() = default;
    virtual ~ReadAndRemoveOnlyQueue() = default;

    // Returns the next task. HasTasks() is assumed to be true.
    virtual const SchedulerIncomingTaskQueueTask& Peek() = 0;

    // Removes and returns the next task. HasTasks() is assumed to be true.
    virtual SchedulerIncomingTaskQueueTask Pop() = 0;

    // Whether this queue has tasks.
    virtual bool HasTasks() = 0;

    // Removes all tasks.
    virtual void Clear() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(ReadAndRemoveOnlyQueue);
  };

  // Provides a read-write task queue.
  class Queue : public ReadAndRemoveOnlyQueue {
   public:
    Queue() = default;
    ~Queue() override = default;

    // Adds the task to the end of the queue.
    virtual void Push(SchedulerIncomingTaskQueueTask pending_task) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Queue);
  };

  SchedulerIncomingTaskQueue(MessageLoop* message_loop,
                             const TaskTraits& task_traits);

  scoped_refptr<SingleThreadTaskRunner> incoming_task_runner();

  // Returns true if the message loop is "idle". Provided for testing.
  bool IsIdleForTesting();

  // Disconnects |this| from the parent message loop.
  void WillDestroyCurrentMessageLoop();

  // This should be called when the message loop becomes ready for
  // scheduling work.
  void StartScheduling();

  // Call this when the message pump is just about to run.
  void OnRunLoopStarting();

  // Called when the message pump is stopping.
  void OnRunLoopStopping();

  // Runs |pending_task|.
  void RunTask(SchedulerIncomingTaskQueueTask* pending_task);

  // Record |pending_task| cancelled.
  void RecordTaskCancelled(SchedulerIncomingTaskQueueTask* task);

  ReadAndRemoveOnlyQueue& triage_tasks() { return triage_tasks_; }

  Queue& delayed_tasks() { return delayed_tasks_; }

  Queue& deferred_tasks() { return deferred_tasks_; }

  void BindToCurrentThread();

  void SetTaskTracker(TaskTracker* task_tracker);

  bool HasPendingHighResolutionTasks() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return pending_high_res_tasks_ > 0;
  }

 private:
  class IncomingTaskRunner;
  friend class RefCountedThreadSafe<SchedulerIncomingTaskQueue>;

  // These queues below support the previous MessageLoop behavior of
  // maintaining three queue queues to process tasks:
  //
  // TriageQueue
  // The first queue to receive all tasks for the processing sequence. Tasks are
  // generally either dispatched immediately or sent to the queues below.
  //
  // DelayedQueue
  // The queue for holding tasks that should be run later and sorted by expected
  // run time.
  //
  // DeferredQueue
  // The queue for holding tasks that couldn't be run while the MessageLoop was
  // nested. These are generally processed during the idle stage.
  //
  // Many of these do not share implementations even though they look like they
  // could because of small quirks (reloading semantics) or differing underlying
  // data strucutre (TaskQueue vs DelayedTaskQueue).

  // The starting point for all tasks on the sequence processing the tasks.
  class TriageQueue : public ReadAndRemoveOnlyQueue {
   public:
    TriageQueue(SchedulerIncomingTaskQueue* outer);
    ~TriageQueue() override;

    // ReadAndRemoveOnlyQueue:
    // In general, the methods below will attempt to reload from the incoming
    // queue if the queue itself is empty except for Clear(). See Clear() for
    // why it doesn't reload.
    const SchedulerIncomingTaskQueueTask& Peek() override;
    SchedulerIncomingTaskQueueTask Pop() override;
    // Whether this queue has tasks after reloading from the incoming queue.
    bool HasTasks() override;
    void Clear() override;

    // TriageQueue:
    void ReloadFromIncomingTaskRunnerIfEmpty();
    void NotifyTaskTrackerOfCurrentTasks();
    void NotifyTaskTrackerOfTaskCancellation();

   private:
    SchedulerIncomingTaskQueue* const outer_;

    circular_deque<SchedulerIncomingTaskQueueTask> triage_queue_;

    DISALLOW_COPY_AND_ASSIGN(TriageQueue);
  };

  class DelayedQueue : public Queue {
   public:
    DelayedQueue(SchedulerIncomingTaskQueue* outer);
    ~DelayedQueue() override;

    // Queue:
    const SchedulerIncomingTaskQueueTask& Peek() override;
    SchedulerIncomingTaskQueueTask Pop() override;
    // Whether this queue has tasks after sweeping the cancelled ones in front.
    bool HasTasks() override;
    void Clear() override;
    void Push(SchedulerIncomingTaskQueueTask pending_task) override;

    // DelayedQueue:
    void NotifyTaskTrackerOfCurrentTasks();
    void NotifyTaskTrackerOfTaskCancellation();

   private:
    SchedulerIncomingTaskQueue* const outer_;
    circular_deque<SchedulerIncomingTaskQueueTask> delayed_queue_;

    DISALLOW_COPY_AND_ASSIGN(DelayedQueue);
  };

  class DeferredQueue : public Queue {
   public:
    DeferredQueue(SchedulerIncomingTaskQueue* outer);
    ~DeferredQueue() override;

    // Queue:
    const SchedulerIncomingTaskQueueTask& Peek() override;
    SchedulerIncomingTaskQueueTask Pop() override;
    bool HasTasks() override;
    void Clear() override;
    void Push(SchedulerIncomingTaskQueueTask pending_task) override;

    // DeferredQueue:
    void NotifyTaskTrackerOfCurrentTasks();
    void NotifyTaskTrackerOfTaskCancellation();

   private:
    SchedulerIncomingTaskQueue* const outer_;
    circular_deque<SchedulerIncomingTaskQueueTask> deferred_queue_;

    DISALLOW_COPY_AND_ASSIGN(DeferredQueue);
  };

  virtual ~SchedulerIncomingTaskQueue();

  // Checks calls made only on the MessageLoop thread.
  SEQUENCE_CHECKER(sequence_checker_);

  MessageLoop* const message_loop_;

  // Queue for initial triaging of tasks on the |sequence_checker_| sequence.
  TriageQueue triage_tasks_;

  // Queue for delayed tasks on the |sequence_checker_| sequence.
  DelayedQueue delayed_tasks_;

  // Queue for non-nestable deferred tasks on the |sequence_checker_| sequence.
  DeferredQueue deferred_tasks_;

  // Number of high resolution tasks in the sequence affine queues above.
  int pending_high_res_tasks_ = 0;

  const scoped_refptr<IncomingTaskRunner> incoming_task_runner_;

  int task_scheduler_version_ = 0;
  TaskTracker* task_tracker_ = nullptr;

  const TaskTraits task_traits_;

  debug::TaskAnnotator task_annotator_;

  SchedulerLock lock_;

  DISALLOW_COPY_AND_ASSIGN(SchedulerIncomingTaskQueue);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SCHEDULER_INCOMING_TASK_QUEUE_H_
