// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/test_mock_time_task_runner.h"

#include <utility>

#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/clock.h"
#include "base/time/tick_clock.h"

namespace base {

namespace {

// MockTickClock --------------------------------------------------------------

// TickClock that always returns the then-current mock time ticks of
// |task_runner| as the current time ticks.
class MockTickClock : public TickClock {
 public:
  explicit MockTickClock(
      scoped_refptr<const TestMockTimeTaskRunner> task_runner);

  // TickClock:
  TimeTicks NowTicks() override;

 private:
  scoped_refptr<const TestMockTimeTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(MockTickClock);
};

MockTickClock::MockTickClock(
    scoped_refptr<const TestMockTimeTaskRunner> task_runner)
    : task_runner_(task_runner) {
}

TimeTicks MockTickClock::NowTicks() {
  return task_runner_->NowTicks();
}

// MockClock ------------------------------------------------------------------

// Clock that always returns the then-current mock time of |task_runner| as the
// current time.
class MockClock : public Clock {
 public:
  explicit MockClock(scoped_refptr<const TestMockTimeTaskRunner> task_runner);

  // Clock:
  Time Now() override;

 private:
  scoped_refptr<const TestMockTimeTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(MockClock);
};

MockClock::MockClock(scoped_refptr<const TestMockTimeTaskRunner> task_runner)
    : task_runner_(task_runner) {
}

Time MockClock::Now() {
  return task_runner_->Now();
}

// A SingleThreadTaskRunner which forwards everything to its |target_|. This is
// useful to break ownership chains when it is known that |target_| will outlive
// the NonOwningProxyTaskRunner it's injected into. In particular,
// TestMockTimeTaskRunner is forced to be ref-counted by virtue of being a
// SingleThreadTaskRunner. As such it is impossible for it to have a
// ThreadTaskRunnerHandle member that points back to itself as the
// ThreadTaskRunnerHandle which it owns would hold a ref back to it. To break
// this dependency cycle, the ThreadTaskRunnerHandle is instead handed a
// NonOwningProxyTaskRunner which allows the TestMockTimeTaskRunner to not hand
// a ref to its ThreadTaskRunnerHandle while promising in return that it will
// outlive that ThreadTaskRunnerHandle instance.
class NonOwningProxyTaskRunner : public SingleThreadTaskRunner {
 public:
  explicit NonOwningProxyTaskRunner(SingleThreadTaskRunner* target)
      : target_(target) {
    DCHECK(target_);
  }

  // SingleThreadTaskRunner:
  bool RunsTasksInCurrentSequence() const override {
    return target_->RunsTasksInCurrentSequence();
  }
  bool PostDelayedTask(const Location& from_here,
                       OnceClosure task,
                       TimeDelta delay) override {
    return target_->PostDelayedTask(from_here, std::move(task), delay);
  }
  bool PostNonNestableDelayedTask(const Location& from_here,
                                  OnceClosure task,
                                  TimeDelta delay) override {
    return target_->PostNonNestableDelayedTask(from_here, std::move(task),
                                               delay);
  }

 private:
  friend class RefCountedThreadSafe<NonOwningProxyTaskRunner>;
  ~NonOwningProxyTaskRunner() override = default;

  SingleThreadTaskRunner* const target_;

  DISALLOW_COPY_AND_ASSIGN(NonOwningProxyTaskRunner);
};

}  // namespace

// TestMockTimeTaskRunner::TestOrderedPendingTask -----------------------------

// Subclass of TestPendingTask which has a strictly monotonically increasing ID
// for every task, so that tasks posted with the same 'time to run' can be run
// in the order of being posted.
struct TestMockTimeTaskRunner::TestOrderedPendingTask
    : public base::TestPendingTask {
  TestOrderedPendingTask();
  TestOrderedPendingTask(const Location& location,
                         OnceClosure task,
                         TimeTicks post_time,
                         TimeDelta delay,
                         size_t ordinal,
                         TestNestability nestability);
  TestOrderedPendingTask(TestOrderedPendingTask&&);
  ~TestOrderedPendingTask();

  TestOrderedPendingTask& operator=(TestOrderedPendingTask&&);

  size_t ordinal;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestOrderedPendingTask);
};

TestMockTimeTaskRunner::TestOrderedPendingTask::TestOrderedPendingTask()
    : ordinal(0) {
}

TestMockTimeTaskRunner::TestOrderedPendingTask::TestOrderedPendingTask(
    TestOrderedPendingTask&&) = default;

TestMockTimeTaskRunner::TestOrderedPendingTask::TestOrderedPendingTask(
    const Location& location,
    OnceClosure task,
    TimeTicks post_time,
    TimeDelta delay,
    size_t ordinal,
    TestNestability nestability)
    : base::TestPendingTask(location,
                            std::move(task),
                            post_time,
                            delay,
                            nestability),
      ordinal(ordinal) {}

TestMockTimeTaskRunner::TestOrderedPendingTask::~TestOrderedPendingTask() =
    default;

TestMockTimeTaskRunner::TestOrderedPendingTask&
TestMockTimeTaskRunner::TestOrderedPendingTask::operator=(
    TestOrderedPendingTask&&) = default;

// TestMockTimeTaskRunner -----------------------------------------------------

// TODO(gab): This should also set the SequenceToken for the current thread.
// Ref. TestMockTimeTaskRunner::RunsTasksInCurrentSequence().
TestMockTimeTaskRunner::ScopedContext::ScopedContext(
    scoped_refptr<TestMockTimeTaskRunner> scope)
    : on_destroy_(ThreadTaskRunnerHandle::OverrideForTesting(scope)) {
  scope->RunUntilIdle();
}

TestMockTimeTaskRunner::ScopedContext::~ScopedContext() = default;

bool TestMockTimeTaskRunner::TemporalOrder::operator()(
    const TestOrderedPendingTask& first_task,
    const TestOrderedPendingTask& second_task) const {
  if (first_task.GetTimeToRun() == second_task.GetTimeToRun())
    return first_task.ordinal > second_task.ordinal;
  return first_task.GetTimeToRun() > second_task.GetTimeToRun();
}

TestMockTimeTaskRunner::TestMockTimeTaskRunner(Type type)
    : TestMockTimeTaskRunner(Time::UnixEpoch(), TimeTicks(), type) {}

TestMockTimeTaskRunner::TestMockTimeTaskRunner(Time start_time,
                                               TimeTicks start_ticks,
                                               Type type)
    : now_(start_time),
      now_ticks_(start_ticks),
      tasks_non_empty_cv_(&tasks_lock_) {
  switch (type) {
    case Type::kStandalone: {
      // No custom setup required.
      break;
    }
    case Type::kBoundToThread: {
      thread_task_runner_handle_ = std::make_unique<ThreadTaskRunnerHandle>(
          MakeRefCounted<NonOwningProxyTaskRunner>(this));
      RunLoop::RegisterDelegateForCurrentThread(this);
      break;
    }
    // Refer to "override scenario" comments throughout this file for
    // documentation on how this mode works.
    case Type::kTakeOverThread: {
      overridden_task_runner_ = ThreadTaskRunnerHandle::Get();
      DCHECK(overridden_task_runner_);
      thread_task_runner_handle_override_scope_ =
          ThreadTaskRunnerHandle::OverrideForTesting(
              MakeRefCounted<NonOwningProxyTaskRunner>(this),
              ThreadTaskRunnerHandle::OverrideType::kTakeOverThread);

      // Override the existing Delegate and tell it to always quit-when-idle
      // unless all of these conditions hold:
      //   1) The Run() call was induced by RunLoop::Run().
      //   2) There are no |tasks_| remaining in this TestMockTimeTaskRunner.
      //   3) This TestMockTimeTaskRunner wasn't itself asked to quit the
      //      topmost Run() instance when idle.
      // When the overridden Delegate keeps control of Run() (i.e. isn't told to
      // quit-when-idle): Run() will block in the overridden Delegate until
      // either it receives work from another source (e.g. system) or this
      // TestMockTimeTaskRunner receives work and wakes up the overridden
      // Delegate by posting a no-op task to it.
      overridden_delegate_ =
          RunLoop::OverrideDelegateForCurrentThreadForTesting(
              this, Bind(
                        [](TestMockTimeTaskRunner* self) {
                          AutoLock scoped_lock(self->tasks_lock_);
                          return !self->runs_were_run_loop_induced_.top() ||
                                 !self->tasks_.empty() ||
                                 self->ShouldQuitWhenIdle();
                        },
                        Unretained(this)));
      DCHECK(overridden_delegate_);
      break;
    }
  }
}

TestMockTimeTaskRunner::~TestMockTimeTaskRunner() = default;

void TestMockTimeTaskRunner::FastForwardBy(TimeDelta delta) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_GE(delta, TimeDelta());

  const TimeTicks original_now_ticks = NowTicks();
  ProcessAllTasksNoLaterThan(delta);
  ForwardClocksUntilTickTime(original_now_ticks + delta);
}

void TestMockTimeTaskRunner::RunUntilIdle() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ProcessAllTasksNoLaterThan(TimeDelta());
}

void TestMockTimeTaskRunner::FastForwardUntilNoTasksRemain() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ProcessAllTasksNoLaterThan(TimeDelta::Max());
}

void TestMockTimeTaskRunner::ClearPendingTasks() {
  DCHECK(thread_checker_.CalledOnValidThread());
  AutoLock scoped_lock(tasks_lock_);
  while (!tasks_.empty())
    tasks_.pop();
}

Time TestMockTimeTaskRunner::Now() const {
  AutoLock scoped_lock(tasks_lock_);
  return now_;
}

TimeTicks TestMockTimeTaskRunner::NowTicks() const {
  AutoLock scoped_lock(tasks_lock_);
  return now_ticks_;
}

std::unique_ptr<Clock> TestMockTimeTaskRunner::GetMockClock() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return std::make_unique<MockClock>(this);
}

std::unique_ptr<TickClock> TestMockTimeTaskRunner::GetMockTickClock() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return std::make_unique<MockTickClock>(this);
}

base::circular_deque<TestPendingTask>
TestMockTimeTaskRunner::TakePendingTasks() {
  AutoLock scoped_lock(tasks_lock_);
  base::circular_deque<TestPendingTask> tasks;
  while (!tasks_.empty()) {
    // It's safe to remove const and consume |task| here, since |task| is not
    // used for ordering the item.
    tasks.push_back(
        std::move(const_cast<TestOrderedPendingTask&>(tasks_.top())));
    tasks_.pop();
  }
  return tasks;
}

bool TestMockTimeTaskRunner::HasPendingTask() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return !tasks_.empty();
}

size_t TestMockTimeTaskRunner::GetPendingTaskCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return tasks_.size();
}

TimeDelta TestMockTimeTaskRunner::NextPendingTaskDelay() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  AutoLock scoped_lock(tasks_lock_);
  return tasks_.empty() ? TimeDelta::Max()
                        : tasks_.top().GetTimeToRun() - NowTicks();
}

// TODO(gab): Combine |thread_checker_| with a SequenceToken to differentiate
// between tasks running in the scope of this TestMockTimeTaskRunner and other
// task runners sharing this thread. http://crbug.com/631186
bool TestMockTimeTaskRunner::RunsTasksInCurrentSequence() const {
  return thread_checker_.CalledOnValidThread();
}

bool TestMockTimeTaskRunner::PostDelayedTask(const Location& from_here,
                                             OnceClosure task,
                                             TimeDelta delay) {
  AutoLock scoped_lock(tasks_lock_);

  const bool was_empty = tasks_.empty();

  tasks_.push(TestOrderedPendingTask(from_here, std::move(task), now_ticks_,
                                     delay, next_task_ordinal_++,
                                     TestPendingTask::NESTABLE));

  // Unblock upon receiving work from idle. Note: this is done after posting the
  // task as some platforms' kernels prioritize the signaled thread (which would
  // have no work if signaled before pushing |task| into |tasks_|).
  if (was_empty) {
    if (overridden_delegate_) {
      // Wake the running overridden Delegate in the override scenario.
      overridden_task_runner_->PostTask(FROM_HERE, Bind(&DoNothing));
    } else {
      // Merely signal the blocked CV in non-override scenarios.
      tasks_non_empty_cv_.Signal();
    }
  }

  return true;
}

bool TestMockTimeTaskRunner::PostNonNestableDelayedTask(
    const Location& from_here,
    OnceClosure task,
    TimeDelta delay) {
  return PostDelayedTask(from_here, std::move(task), delay);
}

void TestMockTimeTaskRunner::OnBeforeSelectingTask() {
  // Empty default implementation.
}

void TestMockTimeTaskRunner::OnAfterTimePassed() {
  // Empty default implementation.
}

void TestMockTimeTaskRunner::OnAfterTaskRun() {
  // Empty default implementation.
}

void TestMockTimeTaskRunner::ProcessAllTasksNoLaterThan(TimeDelta max_delta,
                                                        bool run_loop_induced) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_GE(max_delta, TimeDelta());

  // Multiple test task runners can share the same thread for determinism in
  // unit tests. Make sure this TestMockTimeTaskRunner's tasks run in its scope.
  ScopedClosureRunner undo_override;
  if (!ThreadTaskRunnerHandle::IsSet() ||
      ThreadTaskRunnerHandle::Get() != this) {
    undo_override = ThreadTaskRunnerHandle::OverrideForTesting(this);
  }

  // In the override scenario: first flush any remaining tasks previously handed
  // over to |overridden_delegate_|. This Run() call also guarantees that
  // |overridden_delegate_| tasks from other sources get a chance to run before
  // declaring idle even if |tasks_.empty()|.
  if (overridden_delegate_) {
    runs_were_run_loop_induced_.push(true);
    overridden_delegate_->Run(true);
    runs_were_run_loop_induced_.pop();
  }

  const TimeTicks original_now_ticks = NowTicks();
  while (!quit_run_loop_) {
    OnBeforeSelectingTask();
    TestPendingTask task_info;
    if (!DequeueNextTask(original_now_ticks, max_delta, &task_info))
      break;
    // If tasks were posted with a negative delay, task_info.GetTimeToRun() will
    // be less than |now_ticks_|. ForwardClocksUntilTickTime() takes care of not
    // moving the clock backwards in this case.
    auto task_time = task_info.GetTimeToRun();
    ForwardClocksUntilTickTime(task_time);

    if (overridden_delegate_) {
      // In the override scenario: forward the dequeued task and any other
      // scheduled for the same time to the underlying Delegate and run it.
      do {
        overridden_task_runner_->PostTask(task_info.location,
                                          std::move(task_info.task));
      } while (DequeueNextTask(task_time, TimeDelta(), &task_info));
      // This Run() will return when the underlying Delegate processed all the
      // tasks handed to it (becomes idle) unless
      // |tasks_.empty() && !ShouldQuitWhenIdle()| as declared in the
      // OverrideDelegateForCurrentThreadForTesting() call above.
      runs_were_run_loop_induced_.push(run_loop_induced);
      overridden_delegate_->Run(true);
      runs_were_run_loop_induced_.pop();
    } else {
      // Simply run the dequeued task in the non-override scenario.
      std::move(task_info.task).Run();
    }

    OnAfterTaskRun();
  }
}

void TestMockTimeTaskRunner::ForwardClocksUntilTickTime(TimeTicks later_ticks) {
  DCHECK(thread_checker_.CalledOnValidThread());
  {
    AutoLock scoped_lock(tasks_lock_);
    if (later_ticks <= now_ticks_)
      return;

    now_ += later_ticks - now_ticks_;
    now_ticks_ = later_ticks;
  }
  OnAfterTimePassed();
}

bool TestMockTimeTaskRunner::DequeueNextTask(const TimeTicks& reference,
                                             const TimeDelta& max_delta,
                                             TestPendingTask* next_task) {
  DCHECK(thread_checker_.CalledOnValidThread());
  AutoLock scoped_lock(tasks_lock_);
  if (!tasks_.empty() &&
      (tasks_.top().GetTimeToRun() - reference) <= max_delta) {
    // It's safe to remove const and consume |task| here, since |task| is not
    // used for ordering the item.
    *next_task = std::move(const_cast<TestOrderedPendingTask&>(tasks_.top()));
    tasks_.pop();
    return true;
  }
  return false;
}

void TestMockTimeTaskRunner::Run(bool application_tasks_allowed) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Since TestMockTimeTaskRunner doesn't process system messages: there's no
  // hope for anything but an application task to call Quit(). If this RunLoop
  // can't process application tasks (i.e. disallowed by default in nested
  // RunLoops) it's guaranteed to hang...
  DCHECK(application_tasks_allowed)
      << "This is a nested RunLoop instance and needs to be of "
         "Type::kNestableTasksAllowed.";

  while (!quit_run_loop_) {
    RunUntilIdle();
    if (quit_run_loop_ || ShouldQuitWhenIdle())
      break;

    // Peek into |tasks_| to perform one of two things:
    //   A) If there are no remaining tasks, wait until one is posted and
    //      restart from the top (can't happen in the override scenario, see
    //      below).
    //   B) If there is a remaining delayed task. Fast-forward to reach the next
    //      round of tasks.
    TimeDelta auto_fast_forward_by;
    {
      AutoLock scoped_lock(tasks_lock_);
      if (tasks_.empty()) {
        // This can't happen in the override scenario as the
        // RunUntileIdle()/FastForwardBy() which consumes the last task will
        // remain stuck in |overridden_delegate_->Run()| until a new task
        // arrives per the overriding ShouldQuitWhenIdleCallback used (and this
        // loop didn't start with |tasks_.empty()| per the logic preceding it).
        DCHECK(!overridden_delegate_);
        while (tasks_.empty())
          tasks_non_empty_cv_.Wait();
        continue;
      }
      auto_fast_forward_by = tasks_.top().GetTimeToRun() - now_ticks_;
    }
    // Fast-forward time just enough to run the next set of tasks, then go back
    // to top to check idle conditions again.
    const TimeTicks original_now_ticks = NowTicks();
    ProcessAllTasksNoLaterThan(auto_fast_forward_by, true);
    ForwardClocksUntilTickTime(original_now_ticks + auto_fast_forward_by);
  }
  quit_run_loop_ = false;
}

void TestMockTimeTaskRunner::Quit() {
  DCHECK(thread_checker_.CalledOnValidThread());
  quit_run_loop_ = true;
  if (overridden_delegate_)
    overridden_delegate_->Quit();
}

void TestMockTimeTaskRunner::EnsureWorkScheduled() {
  // Nothing to do in TestMockTimeTaskRunner: TestMockTimeTaskRunner::Run() will
  // always process tasks and doesn't need an extra kick on nested runs.

  if (overridden_delegate_)
    overridden_delegate_->EnsureWorkScheduled();
}

}  // namespace base
