// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MESSAGE_LOOP_INCOMING_TASK_QUEUE_H_
#define BASE_MESSAGE_LOOP_INCOMING_TASK_QUEUE_H_

#include "base/base_export.h"
#include "base/callback.h"
#include "base/debug/task_annotator.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/pending_task.h"
#include "base/sequence_checker.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"

namespace base {

class MessageLoop;
class PostTaskTest;

namespace internal {

// Implements a queue of tasks posted to the message loop running on the current
// thread. This class takes care of synchronizing posting tasks from different
// threads and together with MessageLoop ensures clean shutdown.
class BASE_EXPORT IncomingTaskQueue
    : public RefCountedThreadSafe<IncomingTaskQueue> {
 private:
  class InitialTaskQueuePolicy;
  class DelayedTaskQueuePolicy;
  class DeferredTaskQueuePolicy;

 public:
  template <typename TaskQueuePolicy, typename TaskQueueType>
  class RemoveOnlyQueue {
   public:
    RemoveOnlyQueue(IncomingTaskQueue* outer) : policy_(&queue_, outer) {}

    const PendingTask& Peek() { return policy_.Peek(); }

    PendingTask Pop() { return policy_.Pop(); }

    bool HasTasks() { return policy_.HasTasks(); }

    bool Clear() { return policy_.Clear(); }

   protected:
    TaskQueueType queue_;
    TaskQueuePolicy policy_;

   private:
    DISALLOW_COPY_AND_ASSIGN(RemoveOnlyQueue);
  };

  template <typename TaskQueuePolicy, typename TaskQueueType>
  class Queue : public RemoveOnlyQueue<TaskQueuePolicy, TaskQueueType> {
   public:
    Queue(IncomingTaskQueue* outer)
        : RemoveOnlyQueue<TaskQueuePolicy, TaskQueueType>(outer) {}

    void Push(PendingTask pending_task) {
      // The |this| reference is currently required due to the fact that the
      // superclass is a template.
      this->policy_.Push(std::move(pending_task));
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(Queue);
  };

  explicit IncomingTaskQueue(MessageLoop* message_loop);

  // Appends a task to the incoming queue. Posting of all tasks is routed though
  // AddToIncomingQueue() or TryAddToIncomingQueue() to make sure that posting
  // task is properly synchronized between different threads.
  //
  // Returns true if the task was successfully added to the queue, otherwise
  // returns false. In all cases, the ownership of |task| is transferred to the
  // called method.
  bool AddToIncomingQueue(const tracked_objects::Location& from_here,
                          OnceClosure task,
                          TimeDelta delay,
                          bool nestable);

  // Returns true if the message loop is "idle". Provided for testing.
  bool IsIdleForTesting();

  // Disconnects |this| from the parent message loop.
  void WillDestroyCurrentMessageLoop();

  // This should be called when the message loop becomes ready for
  // scheduling work.
  void StartScheduling();

  // Runs |pending_task|.
  void RunTask(PendingTask* pending_task);

  RemoveOnlyQueue<InitialTaskQueuePolicy, TaskQueue>& initial_tasks() {
    return initial_tasks_;
  }

  Queue<DelayedTaskQueuePolicy, DelayedTaskQueue>& delayed_tasks() {
    return delayed_tasks_;
  }

  Queue<DeferredTaskQueuePolicy, TaskQueue>& deferred_tasks() {
    return deferred_tasks_;
  }

  bool HasPendingHighResolutionTasks();

 private:
  friend class base::PostTaskTest;
  friend class RefCountedThreadSafe<IncomingTaskQueue>;

  class InitialTaskQueuePolicy {
   public:
    InitialTaskQueuePolicy(TaskQueue* queue, IncomingTaskQueue* outer);
    void Push(PendingTask pending_task);
    const PendingTask& Peek();
    PendingTask Pop();
    bool HasTasks();
    bool Clear();

   private:
    void ReloadFromIncomingQueue();

    TaskQueue* const queue_;
    IncomingTaskQueue* const outer_;

    DISALLOW_COPY_AND_ASSIGN(InitialTaskQueuePolicy);
  };

  class DelayedTaskQueuePolicy {
   public:
    DelayedTaskQueuePolicy(DelayedTaskQueue* queue, IncomingTaskQueue* outer);
    void Push(PendingTask pending_task);
    const PendingTask& Peek();
    PendingTask Pop();
    bool HasTasks();
    bool Clear();

   private:
    DelayedTaskQueue* const queue_;
    IncomingTaskQueue* const outer_;

    DISALLOW_COPY_AND_ASSIGN(DelayedTaskQueuePolicy);
  };

  class DeferredTaskQueuePolicy {
   public:
    DeferredTaskQueuePolicy(TaskQueue* queue, IncomingTaskQueue* outer);
    void Push(PendingTask pending_task);
    const PendingTask& Peek();
    PendingTask Pop();
    bool HasTasks();
    bool Clear();

   private:
    TaskQueue* const queue_;
    IncomingTaskQueue* const outer_;

    DISALLOW_COPY_AND_ASSIGN(DeferredTaskQueuePolicy);
  };

  virtual ~IncomingTaskQueue();

  // Adds a task to |incoming_queue_|. The caller retains ownership of
  // |pending_task|, but this function will reset the value of
  // |pending_task->task|. This is needed to ensure that the posting call stack
  // does not retain |pending_task->task| beyond this function call.
  bool PostPendingTask(PendingTask* pending_task);

  // Does the real work of posting a pending task. Returns true if the caller
  // should call ScheduleWork() on the message loop.
  bool PostPendingTaskLockRequired(PendingTask* pending_task);

  // Loads tasks from the |incoming_queue_| into |*work_queue|. Must be called
  // from the thread that is running the loop. Returns the number of tasks that
  // require high resolution timers.
  int ReloadWorkQueue(TaskQueue* work_queue);

  // Checks calls made only on the MessageLoop thread.
  SEQUENCE_CHECKER(sequence_checker_);

  debug::TaskAnnotator task_annotator_;

  // True if we always need to call ScheduleWork when receiving a new task, even
  // if the incoming queue was not empty.
  const bool always_schedule_work_;

  RemoveOnlyQueue<InitialTaskQueuePolicy, TaskQueue> initial_tasks_;
  Queue<DelayedTaskQueuePolicy, DelayedTaskQueue> delayed_tasks_;
  Queue<DeferredTaskQueuePolicy, TaskQueue> deferred_tasks_;

  // How many high resolution tasks are in the pending task queue. This value
  // increases by N every time we call ReloadWorkQueue() and decreases by 1
  // every time we call RunTask() if the task needs a high resolution timer.
  int pending_high_res_tasks_ = 0;

  // Lock that protects |message_loop_| to prevent it from being deleted while
  // a request is made to schedule work.
  base::Lock message_loop_lock_;

  // Points to the message loop that owns |this|.
  MessageLoop* message_loop_;

  // Synchronizes access to all members below this line.
  base::Lock incoming_queue_lock_;

  // Number of tasks that require high resolution timing. This value is kept
  // so that ReloadWorkQueue() completes in constant time.
  int high_res_task_count_ = 0;

  // An incoming queue of tasks that are acquired under a mutex for processing
  // on this instance's thread. These tasks have not yet been been pushed to
  // |message_loop_|.
  TaskQueue incoming_queue_;

  // True if new tasks should be accepted.
  bool accept_new_tasks_ = true;

  // The next sequence number to use for delayed tasks.
  int next_sequence_num_ = 0;

  // True if our message loop has already been scheduled and does not need to be
  // scheduled again until an empty reload occurs.
  bool message_loop_scheduled_ = false;

  // False until StartScheduling() is called.
  bool is_ready_for_scheduling_ = false;

  DISALLOW_COPY_AND_ASSIGN(IncomingTaskQueue);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_MESSAGE_LOOP_INCOMING_TASK_QUEUE_H_
