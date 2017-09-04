// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_RUNNER_H_
#define BASE_TASK_RUNNER_H_

#include <stddef.h>

#include "base/base_export.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"

namespace base {

struct TaskRunnerTraits;

// A TaskRunner is an object that runs posted tasks (in the form of
// Closure objects).  The TaskRunner interface provides a way of
// decoupling task posting from the mechanics of how each task will be
// run.  TaskRunner provides very weak guarantees as to how posted
// tasks are run (or if they're run at all).  In particular, it only
// guarantees:
//
//   - Posting a task will not run it synchronously.  That is, no
//     Post*Task method will call task.Run() directly.
//
//   - Increasing the delay can only delay when the task gets run.
//     That is, increasing the delay may not affect when the task gets
//     run, or it could make it run later than it normally would, but
//     it won't make it run earlier than it normally would.
//
// TaskRunner does not guarantee the order in which posted tasks are
// run, whether tasks overlap, or whether they're run on a particular
// thread.  Also it does not guarantee a memory model for shared data
// between tasks.  (In other words, you should use your own
// synchronization/locking primitives if you need to share data
// between tasks.)
//
// Implementations of TaskRunner should be thread-safe in that all
// methods must be safe to call on any thread.  Ownership semantics
// for TaskRunners are in general not clear, which is why the
// interface itself is RefCountedThreadSafe.
//
// Some theoretical implementations of TaskRunner:
//
//   - A TaskRunner that uses a thread pool to run posted tasks.
//
//   - A TaskRunner that, for each task, spawns a non-joinable thread
//     to run that task and immediately quit.
//
//   - A TaskRunner that stores the list of posted tasks and has a
//     method Run() that runs each runnable task in random order.
class BASE_EXPORT TaskRunner
    : public RefCountedThreadSafe<TaskRunner, TaskRunnerTraits> {
 public:
  // Posts the given task to be run.  Returns true if the task may be
  // run at some point in the future, and false if the task definitely
  // will not be run.
  //
  // Equivalent to PostDelayedTask(from_here, task, 0).
  bool PostTask(const tracked_objects::Location& from_here, OnceClosure task);

  // Like PostTask, but tries to run the posted task only after |delay_ms|
  // has passed. Implementations should use a tick clock, rather than wall-
  // clock time, to implement |delay|.
  virtual bool PostDelayedTask(const tracked_objects::Location& from_here,
                               OnceClosure task,
                               base::TimeDelta delay) = 0;

  // Returns true iff tasks posted to this TaskRunner are sequenced
  // with this call.
  //
  // In particular:
  // - Returns true if this is a SequencedTaskRunner to which the
  //   current task was posted.
  // - Returns true if this is a SequencedTaskRunner bound to the
  //   same sequence as the SequencedTaskRunner to which the current
  //   task was posted.
  // - Returns true if this is a SingleThreadTaskRunner bound to
  //   the current thread.
  // TODO(http://crbug.com/665062):
  //   This API doesn't make sense for parallel TaskRunners.
  //   Introduce alternate static APIs for documentation purposes of "this runs
  //   in pool X", have RunsTasksInCurrentSequence() return false for parallel
  //   TaskRunners, and ultimately move this method down to SequencedTaskRunner.
  virtual bool RunsTasksInCurrentSequence() const = 0;

  // Posts |task| on the current TaskRunner.  On completion, |reply|
  // is posted to the thread that called PostTaskAndReply().  Both
  // |task| and |reply| are guaranteed to be deleted on the thread
  // from which PostTaskAndReply() is invoked.  This allows objects
  // that must be deleted on the originating thread to be bound into
  // the |task| and |reply| Closures.  In particular, it can be useful
  // to use WeakPtr<> in the |reply| Closure so that the reply
  // operation can be canceled. See the following pseudo-code:
  //
  // class DataBuffer : public RefCountedThreadSafe<DataBuffer> {
  //  public:
  //   // Called to add data into a buffer.
  //   void AddData(void* buf, size_t length);
  //   ...
  // };
  //
  //
  // class DataLoader : public SupportsWeakPtr<DataLoader> {
  //  public:
  //    void GetData() {
  //      scoped_refptr<DataBuffer> buffer = new DataBuffer();
  //      target_thread_.task_runner()->PostTaskAndReply(
  //          FROM_HERE,
  //          base::Bind(&DataBuffer::AddData, buffer),
  //          base::Bind(&DataLoader::OnDataReceived, AsWeakPtr(), buffer));
  //    }
  //
  //  private:
  //    void OnDataReceived(scoped_refptr<DataBuffer> buffer) {
  //      // Do something with buffer.
  //    }
  // };
  //
  //
  // Things to notice:
  //   * Results of |task| are shared with |reply| by binding a shared argument
  //     (a DataBuffer instance).
  //   * The DataLoader object has no special thread safety.
  //   * The DataLoader object can be deleted while |task| is still running,
  //     and the reply will cancel itself safely because it is bound to a
  //     WeakPtr<>.
  bool PostTaskAndReply(const tracked_objects::Location& from_here,
                        OnceClosure task,
                        OnceClosure reply);

  // Similar to PostTaskAndReply, posts a |task| on the current TaskRunner,
  // and on completion, |reply| is posted to the thread that called
  // PostAsyncTaskAndReply().
  // The difference is that |task| needs to take a callback, which should be
  // called on completion. So, |task| can be asynchronous, and be completed
  // on next message loop run.
  // Note that the callback passed to |task| can be called at any thread.
  // Also note that, if the callback passed to |task| is destroyed before
  // it is called, a task to destroy |reply| is posted to the original thread.
  // If it fails, it means the original thread/sequence is stopped beforehand,
  // so there is few things to do. Here, leak the |reply|, including its bound
  // arguments.
  // Here is an example pseudo code:
  //
  // void RunSomethingOnTaskRunner(TaskRunner* task_runner) {
  //   task_runner->PostAsyncTaskAndReply(
  //       FROM_HERE,
  //       base::BindOnce(&AsyncTask::Run),
  //       base::BindOnce(&OnReply));
  // }
  //
  // // Living on another thread.
  // class AsyncTask {
  //  public:
  //   static void Run(OnceClosure on_completed) {
  //     new AsyncTask().RunImpl(std::move(on_completed));
  //   }
  //
  //   AsyncTask();
  //   void RunImpl(OnceClosure on_completed) {
  //     ... do some work ...
  //     another_task_runner_->PostTaskAndReply(
  //         FROM_HERE,
  //         base::BindOnce(&DoAnotherThingOnAnotherThread),
  //         base::BindOnce(&RunImplContinued,
  //                        weak_ptr_factory_.GetWeakPtr(),
  //                        std::move(on_completed)));
  //   }
  //
  //   void RunImplContinued(OnceClosure on_completed) {
  //     // Will run after DoAnotherThingOnAnotherThread is completed.
  //     ... do some more work ...
  //     ... and done the task, so reply to the original thread ...
  //     delete this;
  //     std::move(on_completed).Run();
  //   }
  // };
  //
  // void OnReply() {
  //   ... called on the original thread ...
  // }
  //
  // Some more practical example use cases:
  // - On UI thread, it wants to do some network operations. Then,
  //   post a task to IO thread. The network operation on IO thread is async.
  //   When it is done, call |on_completed| callback on IO thread, so that
  //   |reply| is called back on UI thread.
  // - On IO thread, it wants to do some mojo communication.
  //   Post a task to UI thread, and calls Mojo method. The operation is async.
  //   On completion, a callback passed to Mojo method is called on UI thread,
  //   and it calls |on_completed| callback. Finally, the following operation
  //   is called on IO thread.
  bool PostAsyncTaskAndReply(const tracked_objects::Location& from_here,
                             OnceCallback<void(OnceClosure)> task,
                             OnceClosure reply);

 protected:
  friend struct TaskRunnerTraits;

  // Only the Windows debug build seems to need this: see
  // http://crbug.com/112250.
  friend class RefCountedThreadSafe<TaskRunner, TaskRunnerTraits>;

  TaskRunner();
  virtual ~TaskRunner();

  // Called when this object should be destroyed.  By default simply
  // deletes |this|, but can be overridden to do something else, like
  // delete on a certain thread.
  virtual void OnDestruct() const;
};

struct BASE_EXPORT TaskRunnerTraits {
  static void Destruct(const TaskRunner* task_runner);
};

}  // namespace base

#endif  // BASE_TASK_RUNNER_H_
