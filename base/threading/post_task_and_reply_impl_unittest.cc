// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/debug/stack_trace.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/test_mock_time_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace base {
namespace internal {

namespace {

class PostTaskAndReplyTaskRunner : public internal::PostTaskAndReplyImpl {
 public:
  explicit PostTaskAndReplyTaskRunner(TaskRunner* destination)
      : destination_(destination) {}

 private:
  bool PostTask(const Location& from_here, OnceClosure task) override {
    return destination_->PostTask(from_here, std::move(task));
  }

  // Non-owning.
  TaskRunner* const destination_;
};

class ObjectToDelete : public RefCounted<ObjectToDelete> {
 public:
  // |delete_flag| is set to true when this object is deleted
  ObjectToDelete(bool* delete_flag) : delete_flag_(delete_flag) {
    EXPECT_FALSE(*delete_flag_);
  }

 private:
  friend class RefCounted<ObjectToDelete>;
  ~ObjectToDelete() { *delete_flag_ = true; }

  bool* const delete_flag_;

  DISALLOW_COPY_AND_ASSIGN(ObjectToDelete);
};

class MockObject {
 public:
  MockObject() = default;

  MOCK_METHOD1(Task, void(scoped_refptr<ObjectToDelete>));
  MOCK_METHOD1(Reply, void(scoped_refptr<ObjectToDelete>));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockObject);
};

// A TestMockTimeTaskRunner bound to the current thread that always returns
// false from RunsTasksOnCurrentSequence().
class PretendTasksDontRunOnCurrentSequenceTaskRunner
    : public TestMockTimeTaskRunner {
 public:
  PretendTasksDontRunOnCurrentSequenceTaskRunner()
      : TestMockTimeTaskRunner(Type::kBoundToThread) {}

  // TaskRunner:
  bool RunsTasksInCurrentSequence() const override { return false; }

 private:
  ~PretendTasksDontRunOnCurrentSequenceTaskRunner() override = default;

  DISALLOW_COPY_AND_ASSIGN(PretendTasksDontRunOnCurrentSequenceTaskRunner);
};

}  // namespace

TEST(PostTaskAndReplyImplTest, PostTaskAndReply) {
  auto post_runner = MakeRefCounted<TestMockTimeTaskRunner>();
  auto reply_runner = MakeRefCounted<TestMockTimeTaskRunner>(
      TestMockTimeTaskRunner::Type::kBoundToThread);

  testing::StrictMock<MockObject> mock_object;
  bool task_deleted = false;
  bool reply_deleted = false;

  EXPECT_TRUE(
      PostTaskAndReplyTaskRunner(post_runner.get())
          .PostTaskAndReply(
              FROM_HERE,
              BindOnce(&MockObject::Task, Unretained(&mock_object),
                       MakeRefCounted<ObjectToDelete>(&task_deleted)),
              BindOnce(&MockObject::Reply, Unretained(&mock_object),
                       MakeRefCounted<ObjectToDelete>(&reply_deleted))));

  // Expect the task to be posted to |post_runner|.
  EXPECT_TRUE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());
  EXPECT_FALSE(task_deleted);

  EXPECT_CALL(mock_object, Task(_));
  post_runner->RunUntilIdle();
  testing::Mock::VerifyAndClear(&mock_object);

  // |task| should have been deleted right after being run.
  EXPECT_TRUE(task_deleted);

  // Expect the reply to be posted to |reply_runner|.
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_TRUE(reply_runner->HasPendingTask());

  EXPECT_CALL(mock_object, Reply(_));
  reply_runner->RunUntilIdle();
  testing::Mock::VerifyAndClear(&mock_object);
  EXPECT_TRUE(task_deleted);

  // Expect no pending task in |post_runner| and |reply_runner|.
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());
}

// Verify that the "reply" is deleted on the origin TaskRunner when the "task"
// can't run.
TEST(PostTaskAndReplyImplTest, DeleteReplyWhenTaskIsCanceled) {
  auto post_runner = MakeRefCounted<TestMockTimeTaskRunner>();
  auto reply_runner =
      MakeRefCounted<PretendTasksDontRunOnCurrentSequenceTaskRunner>();

  testing::StrictMock<MockObject> mock_object;
  bool task_deleted = false;
  bool reply_deleted = false;

  EXPECT_TRUE(
      PostTaskAndReplyTaskRunner(post_runner.get())
          .PostTaskAndReply(
              FROM_HERE,
              BindOnce(&MockObject::Task, Unretained(&mock_object),
                       MakeRefCounted<ObjectToDelete>(&task_deleted)),
              BindOnce(&MockObject::Reply, Unretained(&mock_object),
                       MakeRefCounted<ObjectToDelete>(&reply_deleted))));

  EXPECT_FALSE(task_deleted);
  EXPECT_FALSE(reply_deleted);

  post_runner->ClearPendingTasks();
  EXPECT_TRUE(task_deleted);
  EXPECT_FALSE(reply_deleted);

  reply_runner->RunUntilIdle();
  EXPECT_TRUE(task_deleted);
  EXPECT_TRUE(reply_deleted);
}

}  // namespace internal
}  // namespace base
