// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/post_only_owner.h"

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class PostOnlyTest : public ::testing::Test {
 public:
  // Helpful values that our test classes use.
  enum {
    kInitialValue = 0,
    kDifferentValue = 1,

    // Values used by the Derived class.
    kDerivedCtorValue = 111,
    kDerivedDtorValue = 222,

    // Values used by the Other class.
    kOtherCtorValue = 333,
    kOtherDtorValue = 444,
  };

  void SetUp() override { task_runner_ = base::ThreadTaskRunnerHandle::Get(); }

  // Base class that has an intentionally non-virtual destructor.  Casting
  // derived classes to NonVirtualBase should call ~NonVirtualBase only.
  class NonVirtualBase {
   public:
    ~NonVirtualBase() {}
  };

  // Do-nothing base class, just so we can test assignment of derived classes.
  // It introduces a virtual destructor, so that casting derived classes to
  // Base should still use the appropriate (virtual) destructor.
  class Base : public NonVirtualBase {
   public:
    virtual ~Base() {}
  };

  // Handy class to set an int ptr to different values, to verify that things
  // are being run properly.
  class Derived : public Base {
    // It's okay to have protected ctors.
   protected:
    Derived(int* ptr) : ptr_(ptr) { *ptr_ = kDerivedCtorValue; }

   public:
    ~Derived() override { *ptr_ = kDerivedDtorValue; }

    void SetValue(int value) { *ptr_ = value; }

    int GetValue() const { return *ptr_; }

    int* ptr_;
  };

  // Another base class, which sets ints to different values.
  class Other {
   public:
    Other(int* ptr) : ptr_(ptr) { *ptr = kOtherCtorValue; };
    // Virtually inherit, so that all destructors in MultiplyDerived work.
    virtual ~Other() { *ptr_ = kOtherDtorValue; }
    int* ptr_;
  };

  class MultiplyDerived : public Other, public Derived {
   public:
    MultiplyDerived(int* ptr1, int* ptr2) : Other(ptr1), Derived(ptr2) {}
  };

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  int value = kInitialValue;
};

TEST_F(PostOnlyTest, ConstructThenPostThenReset) {
  PostOnlyOwner<Derived> derived(task_runner_, &value);
  EXPECT_FALSE(derived.is_null());

  // Nothing should happen until we run the message loop.
  EXPECT_EQ(value, kInitialValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedCtorValue);

  // Post now that the object has been constructed.
  derived.Post(FROM_HERE, &Derived::SetValue, kDifferentValue);
  EXPECT_EQ(value, kDerivedCtorValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDifferentValue);

  // Call and ignore a return result, just to make sure that it compiles.
  derived.Post(FROM_HERE, &Derived::GetValue);

  // Reset it, and make sure that destruction is posted.  The owner should
  // report that it is null immediately.
  derived.reset();
  EXPECT_TRUE(derived.is_null());
  EXPECT_EQ(value, kDifferentValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedDtorValue);
}

TEST_F(PostOnlyTest, PostBeforeConstruction) {
  // Construct an object and post a message to it, before construction has been
  // run on |task_runner_|.
  PostOnlyOwner<Derived> derived(task_runner_, &value);
  derived.Post(FROM_HERE, &Derived::SetValue, kDifferentValue);
  EXPECT_EQ(value, kInitialValue);
  // Both construction and SetValue should run.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDifferentValue);
}

TEST_F(PostOnlyTest, MoveConstructionFromSameClass) {
  // Verify that we can move-construct with the same class.
  PostOnlyOwner<Derived> derived_old(task_runner_, &value);
  PostOnlyOwner<Derived> derived_new(std::move(derived_old));
  EXPECT_TRUE(derived_old.is_null());
  EXPECT_FALSE(derived_new.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedCtorValue);

  // Verify that |derived_new| owns the object now, and that the virtual
  // destructor is called.
  derived_new = PostOnlyOwner<Derived>();
  EXPECT_EQ(value, kDerivedCtorValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedDtorValue);
}

TEST_F(PostOnlyTest, MoveConstructionFromDerivedClass) {
  // Verify that we can move-construct to a base class from a derived class.
  PostOnlyOwner<Derived> derived(task_runner_, &value);
  PostOnlyOwner<Base> base(std::move(derived));
  EXPECT_TRUE(derived.is_null());
  EXPECT_FALSE(base.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedCtorValue);

  // Verify that |base| owns the object now, and that destruction still destroys
  // Derived properly.
  base = PostOnlyOwner<Base>();
  EXPECT_EQ(value, kDerivedCtorValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedDtorValue);
}

TEST_F(PostOnlyTest, MoveAssignmentFromSameClass) {
  // Test move-assignment using the same classes.
  PostOnlyOwner<Derived> derived_old(task_runner_, &value);
  PostOnlyOwner<Derived> derived_new;

  derived_new = std::move(derived_old);
  EXPECT_TRUE(derived_old.is_null());
  EXPECT_FALSE(derived_new.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedCtorValue);

  // Verify that |derived_new| owns the object now.
  derived_new = PostOnlyOwner<Derived>();
  EXPECT_EQ(value, kDerivedCtorValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedDtorValue);
}

TEST_F(PostOnlyTest, MoveAssignmentFromDerivedClass) {
  // Move-assignment from a derived class to a base class.
  PostOnlyOwner<Derived> derived(task_runner_, &value);
  PostOnlyOwner<Base> base;

  base = std::move(derived);
  EXPECT_TRUE(derived.is_null());
  EXPECT_FALSE(base.is_null());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedCtorValue);

  // Verify that |base| owns the object now, and that destruction still destroys
  // Derived properly.
  base = PostOnlyOwner<Base>();
  EXPECT_EQ(value, kDerivedCtorValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedDtorValue);
}

TEST_F(PostOnlyTest, ReplyWithResult) {
  PostOnlyOwner<Derived> derived(task_runner_, &value);
  int reply = kInitialValue;
  auto reply_cb =
      base::BindOnce([](int* reply, int value) { *reply = value; }, &reply);
  derived.PostAndReply(FROM_HERE, std::move(reply_cb), &Derived::GetValue);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(reply, kDerivedCtorValue);
}

TEST_F(PostOnlyTest, NonVirtualDestructorIsCalled) {
  // Casting to a base class with a non-virtual destructor should use the base
  // class destructor, just like casting then using delete.  Of course, one
  // probably doesn't want to do this in practice, but we want to match existing
  // behavior if it does.
  PostOnlyOwner<Derived> derived(task_runner_, &value);
  PostOnlyOwner<NonVirtualBase> non_virtual_base = std::move(derived);

  non_virtual_base.reset();
  // Should not change |value|, since ~NonVirtualBase has a non-virtual
  // destructor.  Of course, you probably don't want to really do this.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(value, kDerivedCtorValue);
}

TEST_F(PostOnlyTest, MultiplyDerviedDestructionWorksLeftSuper) {
  // Verify that everything works when we're casting around in ways that might
  // change the address.  We cast to the left side of MultiplyDerived and then
  // reset the owner.  ASAN will catch free() errors.
  int value2 = kInitialValue;
  PostOnlyOwner<MultiplyDerived> mderived(task_runner_, &value, &value2);
  PostOnlyOwner<Other> other = std::move(mderived);

  other.reset();
  base::RunLoop().RunUntilIdle();

  // Both destructors should have run.
  EXPECT_EQ(value, kOtherDtorValue);
  EXPECT_EQ(value2, kDerivedDtorValue);
}

TEST_F(PostOnlyTest, MultiplyDerviedDestructionWorksRightSuper) {
  // Verify that everything works when we're casting around in ways that might
  // change the address.  We cast to the right side of MultiplyDerived and then
  // reset the owner.  ASAN will catch free() errors.
  int value2 = kInitialValue;
  PostOnlyOwner<MultiplyDerived> mderived(task_runner_, &value, &value2);
  PostOnlyOwner<Base> base = std::move(mderived);

  base.reset();
  base::RunLoop().RunUntilIdle();

  // Both destructors should have run.
  EXPECT_EQ(value, kOtherDtorValue);
  EXPECT_EQ(value2, kDerivedDtorValue);
}

}  // namespace media
