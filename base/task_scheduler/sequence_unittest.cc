// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/sequence.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/test/gtest_util.h"
#include "base/test/test_simple_task_runner.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {

namespace {

Task CreateTaskWithTaskRunner(
    scoped_refptr<SingleThreadTaskRunner> task_runner) {
  Task task(FROM_HERE, BindOnce(&DoNothing), {TaskPriority::BACKGROUND},
            TimeDelta());
  // |single_thread_task_runner_ref| is used by this test to track a Task
  // object as it is moved.
  task.single_thread_task_runner_ref = std::move(task_runner);
  return task;
}

}  // namespace

TEST(TaskSchedulerSequenceTest, PushTakeRemove) {
  const scoped_refptr<SingleThreadTaskRunner> tr_a =
      MakeRefCounted<TestSimpleTaskRunner>();
  const scoped_refptr<SingleThreadTaskRunner> tr_b =
      MakeRefCounted<TestSimpleTaskRunner>();
  const scoped_refptr<SingleThreadTaskRunner> tr_c =
      MakeRefCounted<TestSimpleTaskRunner>();
  const scoped_refptr<SingleThreadTaskRunner> tr_d =
      MakeRefCounted<TestSimpleTaskRunner>();
  const scoped_refptr<SingleThreadTaskRunner> tr_e =
      MakeRefCounted<TestSimpleTaskRunner>();

  scoped_refptr<Sequence> sequence = MakeRefCounted<Sequence>();

  // Push task A in the sequence. PushTask() should return true since it's the
  // first task->
  EXPECT_TRUE(sequence->PushTask(CreateTaskWithTaskRunner(tr_a)));

  // Push task B, C and D in the sequence. PushTask() should return false since
  // there is already a task in a sequence.
  EXPECT_FALSE(sequence->PushTask(CreateTaskWithTaskRunner(tr_b)));
  EXPECT_FALSE(sequence->PushTask(CreateTaskWithTaskRunner(tr_c)));
  EXPECT_FALSE(sequence->PushTask(CreateTaskWithTaskRunner(tr_d)));

  // Take the task in front of the sequence. It should be task A.
  Optional<Task> task = sequence->TakeTask();
  EXPECT_EQ(task->single_thread_task_runner_ref, tr_a);
  EXPECT_FALSE(task->sequenced_time.is_null());

  // Remove the empty slot. Task B should now be in front.
  EXPECT_FALSE(sequence->Pop());
  task = sequence->TakeTask();
  EXPECT_EQ(task->single_thread_task_runner_ref, tr_b);
  EXPECT_FALSE(task->sequenced_time.is_null());

  // Remove the empty slot. Task C should now be in front.
  EXPECT_FALSE(sequence->Pop());
  task = sequence->TakeTask();
  EXPECT_EQ(task->single_thread_task_runner_ref, tr_c);
  EXPECT_FALSE(task->sequenced_time.is_null());

  // Remove the empty slot.
  EXPECT_FALSE(sequence->Pop());

  // Push task E in the sequence.
  EXPECT_FALSE(sequence->PushTask(CreateTaskWithTaskRunner(tr_e)));

  // Task D should be in front.
  task = sequence->TakeTask();
  EXPECT_EQ(task->single_thread_task_runner_ref, tr_d);
  EXPECT_FALSE(task->sequenced_time.is_null());

  // Remove the empty slot. Task E should now be in front.
  EXPECT_FALSE(sequence->Pop());
  task = sequence->TakeTask();
  EXPECT_EQ(task->single_thread_task_runner_ref, tr_e);
  EXPECT_FALSE(task->sequenced_time.is_null());

  // Remove the empty slot. The sequence should now be empty.
  EXPECT_TRUE(sequence->Pop());
}

TEST(TaskSchedulerSequenceTest, GetSortKey) {
  Task background_task(FROM_HERE, BindOnce(&DoNothing),
                       {TaskPriority::BACKGROUND}, TimeDelta());
  const scoped_refptr<SingleThreadTaskRunner> background_task_runner =
      MakeRefCounted<TestSimpleTaskRunner>();
  background_task.single_thread_task_runner_ref = background_task_runner;
  scoped_refptr<Sequence> background_sequence = MakeRefCounted<Sequence>();
  background_sequence->PushTask(std::move(background_task));
  const SequenceSortKey background_sort_key = background_sequence->GetSortKey();
  auto take_background_task = background_sequence->TakeTask();
  EXPECT_EQ(TaskPriority::BACKGROUND, background_sort_key.priority());
  EXPECT_EQ(take_background_task->sequenced_time,
            background_sort_key.next_task_sequenced_time());

  Task foreground_task(FROM_HERE, BindOnce(&DoNothing),
                       {TaskPriority::USER_VISIBLE}, TimeDelta());
  const scoped_refptr<SingleThreadTaskRunner> foreground_task_runner =
      MakeRefCounted<TestSimpleTaskRunner>();
  foreground_task.single_thread_task_runner_ref = foreground_task_runner;
  scoped_refptr<Sequence> foreground_sequence = MakeRefCounted<Sequence>();
  foreground_sequence->PushTask(std::move(foreground_task));
  const SequenceSortKey foreground_sort_key = foreground_sequence->GetSortKey();
  auto take_foreground_task = foreground_sequence->TakeTask();
  EXPECT_EQ(TaskPriority::USER_VISIBLE, foreground_sort_key.priority());
  EXPECT_EQ(take_foreground_task->sequenced_time,
            foreground_sort_key.next_task_sequenced_time());
}

// Verify that a DCHECK fires if Pop() is called on a sequence whose front slot
// isn't empty.
TEST(TaskSchedulerSequenceTest, PopNonEmptyFrontSlot) {
  scoped_refptr<Sequence> sequence = MakeRefCounted<Sequence>();
  sequence->PushTask(
      Task(FROM_HERE, Bind(&DoNothing), TaskTraits(), TimeDelta()));

  EXPECT_DCHECK_DEATH({ sequence->Pop(); });
}

// Verify that a DCHECK fires if TakeTask() is called on a sequence whose front
// slot is empty.
TEST(TaskSchedulerSequenceTest, TakeEmptyFrontSlot) {
  scoped_refptr<Sequence> sequence = MakeRefCounted<Sequence>();
  sequence->PushTask(
      Task(FROM_HERE, Bind(&DoNothing), TaskTraits(), TimeDelta()));

  EXPECT_TRUE(sequence->TakeTask());
  EXPECT_DCHECK_DEATH({ sequence->TakeTask(); });
}

// Verify that a DCHECK fires if TakeTask() is called on an empty sequence.
TEST(TaskSchedulerSequenceTest, TakeEmptySequence) {
  scoped_refptr<Sequence> sequence = MakeRefCounted<Sequence>();
  EXPECT_DCHECK_DEATH({ sequence->TakeTask(); });
}

}  // namespace internal
}  // namespace base
