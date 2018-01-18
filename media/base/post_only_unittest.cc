// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/post_only.h"

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class PostOnlyTest : public ::testing::Test {
 public:
  void SetUp() override { task_runner_ = base::ThreadTaskRunnerHandle::Get(); }

  // Do-nothing base class, just so we can test assignment of derived classes.
  class Base {
   public:
    virtual ~Base() {}
  };

  // Handy class to set an int ptr to different values, to verify that things
  // are being run properly.
  class Derived : public Base {
    // It's okay to have protected ctors.
   protected:
    Derived(int* ptr, int value) : ptr_(ptr) { *ptr_ = value; }

   public:
    ~Derived() override { *ptr_ = 0; }

    void SetValue(int value) { *ptr_ = value; }

    int GetValue() const { return *ptr_; }

    int* ptr_;
  };

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

TEST_F(PostOnlyTest, ConstructThenPostThenReset) {
  int value = 0;
  PostOnlyOwner<Derived> derived(task_runner_, &value, 123);
  EXPECT_FALSE(derived.is_null());
  // Nothing should happen until we run the message loop.
  EXPECT_EQ(value, 0);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 123);

  // Post now that the object has been constructed.
  derived.Post(FROM_HERE, &Derived::SetValue, 456);
  EXPECT_EQ(value, 123);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 456);

  // Call and ignore a return result, just to make sure that it compiles.
  derived.Post(FROM_HERE, &Derived::GetValue);

  // Reset it, and make sure that destruction is posted.  The owner should
  // report that it is null immediately.
  derived.reset();
  EXPECT_TRUE(derived.is_null());
  EXPECT_EQ(value, 456);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 0);
}

TEST_F(PostOnlyTest, PostBeforeConstruction) {
  int value = 0;
  PostOnlyOwner<Derived> derived(task_runner_, &value, 123);
  derived.Post(FROM_HERE, &Derived::SetValue, 456);
  EXPECT_EQ(value, 0);
  // Both construction and SetValue should run.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 456);
}

TEST_F(PostOnlyTest, MoveConstructionFromSameClass) {
  int value = 0;
  PostOnlyOwner<Derived> derived_old(task_runner_, &value, 123);
  PostOnlyOwner<Derived> derived_new(std::move(derived_old));
  EXPECT_TRUE(derived_old.is_null());
  EXPECT_FALSE(derived_new.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 123);

  // Verify that |derived_new| owns the object now.
  derived_new = PostOnlyOwner<Derived>();
  EXPECT_EQ(value, 123);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 0);
}

TEST_F(PostOnlyTest, MoveConstructionFromDerivedClass) {
  int value = 0;
  PostOnlyOwner<Derived> derived(task_runner_, &value, 123);
  PostOnlyOwner<Base> base(std::move(derived));
  EXPECT_TRUE(derived.is_null());
  EXPECT_FALSE(base.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 123);

  // Verify that |base| owns the object now, and that destruction still destroys
  // Derived properly.
  base = PostOnlyOwner<Base>();
  EXPECT_EQ(value, 123);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 0);
}

TEST_F(PostOnlyTest, MoveAssignment) {
  int value = 0;
  PostOnlyOwner<Derived> derived_old(task_runner_, &value, 123);
  PostOnlyOwner<Derived> derived_new;

  derived_new = std::move(derived_old);
  EXPECT_TRUE(derived_old.is_null());
  EXPECT_FALSE(derived_new.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 123);

  // Verify that |derived_new| owns the object now.
  derived_new = PostOnlyOwner<Derived>();
  EXPECT_EQ(value, 123);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 0);
}

TEST_F(PostOnlyTest, MoveAssignmentFromDerivedClass) {
  int value = 0;
  PostOnlyOwner<Derived> derived(task_runner_, &value, 123);
  PostOnlyOwner<Base> base;

  base = std::move(derived);
  EXPECT_TRUE(derived.is_null());
  EXPECT_FALSE(base.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 123);

  // Verify that |base| owns the object now, and that destruction still destroys
  // Derived properly.
  base = PostOnlyOwner<Base>();
  EXPECT_EQ(value, 123);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, 0);
}

TEST_F(PostOnlyTest, ReplyWithResult) {
  int value = 0;
  PostOnlyOwner<Derived> derived(task_runner_, &value, 123);
  int reply = 0;
  auto reply_cb =
      base::BindOnce([](int* reply, int value) { *reply = value; }, &reply);
  derived.PostAndReply(FROM_HERE, std::move(reply_cb), &Derived::GetValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(reply, 123);
}

}  // namespace media
