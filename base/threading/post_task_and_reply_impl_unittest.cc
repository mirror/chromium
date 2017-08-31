// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
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
  bool PostTask(const tracked_objects::Location& from_here,
                OnceClosure task) override {
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
  MOCK_METHOD0(Reply, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockObject);
};

}  // namespace

TEST(PostTaskAndReplyImplTest, PostTaskAndReply) {
  scoped_refptr<TestSimpleTaskRunner> post_runner(new TestSimpleTaskRunner);
  scoped_refptr<TestSimpleTaskRunner> reply_runner(new TestSimpleTaskRunner);
  ThreadTaskRunnerHandle task_runner_handle(reply_runner);

  testing::StrictMock<MockObject> mock_object;
  bool delete_flag = false;

  EXPECT_TRUE(
      PostTaskAndReplyTaskRunner(post_runner.get())
          .PostTaskAndReply(
              FROM_HERE,
              BindOnce(&MockObject::Task, Unretained(&mock_object),
                       make_scoped_refptr(new ObjectToDelete(&delete_flag))),
              BindOnce(&MockObject::Reply, Unretained(&mock_object))));

  // Expect the task to be posted to |post_runner|.
  EXPECT_TRUE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());
  EXPECT_FALSE(delete_flag);

  EXPECT_CALL(mock_object, Task(_));
  post_runner->RunUntilIdle();
  testing::Mock::VerifyAndClear(&mock_object);

  // |task| should have been deleted right after being run.
  EXPECT_TRUE(delete_flag);

  // Expect the reply to be posted to |reply_runner|.
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_TRUE(reply_runner->HasPendingTask());

  EXPECT_CALL(mock_object, Reply());
  reply_runner->RunUntilIdle();
  testing::Mock::VerifyAndClear(&mock_object);
  EXPECT_TRUE(delete_flag);

  // Expect no pending task in |post_runner| and |reply_runner|.
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());
}

TEST(PostTaskAndReplyImplTest, PostAsyncTaskAndReply) {
  scoped_refptr<TestSimpleTaskRunner> post_runner(new TestSimpleTaskRunner());
  scoped_refptr<TestSimpleTaskRunner> reply_runner(new TestSimpleTaskRunner());
  ThreadTaskRunnerHandle task_runner_handle(reply_runner);

  OnceClosure on_completed;
  bool reply_called = false;
  ASSERT_TRUE(PostTaskAndReplyTaskRunner(post_runner.get())
                  .PostAsyncTaskAndReply(
                      FROM_HERE,
                      BindOnce(
                          [](OnceClosure* on_completed_storage,
                             OnceClosure on_completed) {
                            *on_completed_storage = std::move(on_completed);
                          },
                          Unretained(&on_completed)),
                      BindOnce([](bool* called) { *called = true; },
                               Unretained(&reply_called))));

  // Expect that the |task| is posted to |post_runner|.
  EXPECT_TRUE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());

  // Run the task. Then, |on_completed| should keep the completion callback.
  // Because |on_completed| is not yet called, reply is not yet posted
  // to |reply_runner|.
  post_runner->RunUntilIdle();
  EXPECT_FALSE(on_completed.is_null());
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());

  // Run |on_completed| on |post_runner|.
  post_runner->PostTask(FROM_HERE, std::move(on_completed));
  EXPECT_TRUE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());
  post_runner->RunUntilIdle();
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_TRUE(reply_runner->HasPendingTask());

  // Make sure |reply| is not yet called.
  EXPECT_FALSE(reply_called);

  reply_runner->RunUntilIdle();
  EXPECT_TRUE(reply_called);

  // Expect no pending task in |post_runner| and |reply_runner|.
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());
}

TEST(PostTaskAndReplyImplTest,
     PostAsyncTaskAndReply_CallbackDeletedBeforeCalled) {
  scoped_refptr<TestSimpleTaskRunner> post_runner(new TestSimpleTaskRunner());
  scoped_refptr<TestSimpleTaskRunner> reply_runner(new TestSimpleTaskRunner());
  ThreadTaskRunnerHandle task_runner_handle(reply_runner);

  bool object_deleted = false;
  bool reply_called = false;
  ASSERT_TRUE(
      PostTaskAndReplyTaskRunner(post_runner.get())
          .PostAsyncTaskAndReply(
              FROM_HERE, BindOnce([](OnceClosure on_completed) {
                // Do nothing. So, |on_completed| will never be called.
              }),
              BindOnce([](scoped_refptr<ObjectToDelete> obj,
                          bool* called) { *called = true; },
                       make_scoped_refptr(new ObjectToDelete(&object_deleted)),
                       Unretained(&reply_called))));

  // Expect that the |task| is posted to |post_runner|.
  EXPECT_TRUE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());

  // Run |task|. Then, because |on_completed| callback is destroyed before
  // called. It should post a task to delete the |reply| on |reply_runner|.
  post_runner->RunUntilIdle();
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_TRUE(reply_runner->HasPendingTask());

  // Make sure |reply| is not called, and |obj| is not yet deleted.
  EXPECT_FALSE(object_deleted);
  EXPECT_FALSE(reply_called);

  // Then run |reply_runner|. It should delete the |reply| without calling it.
  reply_runner->RunUntilIdle();
  EXPECT_TRUE(object_deleted);
  EXPECT_FALSE(reply_called);

  // Expect no pending task in |post_runner| and |reply_runner|.
  EXPECT_FALSE(post_runner->HasPendingTask());
  EXPECT_FALSE(reply_runner->HasPendingTask());
}

}  // namespace internal
}  // namespace base
