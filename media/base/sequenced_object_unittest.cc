// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/test/scoped_task_environment.h"
#include "media/base/gmock_callback_support.h"
#include "media/base/sequenced_object.h"
#include "media/base/sequenced_scoped_ref_ptr.h"
#include "media/base/sequenced_unique_ptr.h"
#include "media/base/sequenced_weak_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::DoAll;
using testing::Return;

namespace media {

// This class tests all the SequencedObject stuff.
class SequencedObjectTest : public testing::Test {
 public:
  class TestObject : public SequencedObject<TestObject> {
   public:
    TestObject() {}
    TestObject(int value) : value_(value) {}

    // Do nothing but accept a value.
    MOCK_METHOD1(MethodWithInt, void(int));

    void SetValue(int value) { value_ = value; }

    int value_ = 0;
  };

  class DerivedObject : public TestObject {};

  SequencedObjectTest() {}
  ~SequencedObjectTest() override {}

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::MessageLoop message_loop_;

  DISALLOW_COPY_AND_ASSIGN(SequencedObjectTest);
};

TEST_F(SequencedObjectTest, WeakRefsPostToProperThread) {
  // Construct a SequencedObject on the current loop, and make sure that weak
  // refs post to the right thread.
  std::unique_ptr<TestObject> test = base::MakeUnique<TestObject>();
  SequencedWeakPtr<TestObject> swp(test);

  // Check that the method is not called until we RunUntilIdle.
  const int value = 123;
  EXPECT_CALL(*test, MethodWithInt(_)).Times(0);
  swp.BindOnce(&TestObject::MethodWithInt).Run(value);
  testing::Mock::VerifyAndClearExpectations(test.get());
  EXPECT_CALL(*test, MethodWithInt(value)).Times(1);
  scoped_task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(test.get());

  // Also try it after we delete the object.
  // TODO(liberato): how to we test that it did nothing?  Hopefully the *SAN
  // variants will catch problems.
  test.reset();
  swp.BindOnce(&TestObject::MethodWithInt).Run(value);

  // TODO(liberato): verify that SWPs are copyable.
}

TEST_F(SequencedObjectTest, RefPtrConstructsOnProperThread) {
  // Test that refptr constructs on the proper thread with args.
  const int value = 123;
  SequencedScopedRefPtr<TestObject> test_ref(
      base::SequencedTaskRunnerHandle::Get(), value);
  // Verify that there's no object yet.
  ASSERT_EQ(test_ref.GetRawPointerForTesting(), nullptr);
  // |test_ref| should own an object.
  ASSERT_TRUE((bool)test_ref);
  scoped_task_environment_.RunUntilIdle();

  // Now that we've RunUntilIdle, we should have an object.
  TestObject* test = test_ref.GetRawPointerForTesting();
  ASSERT_NE(test, nullptr);
  ASSERT_EQ(test->value_, value);
}

TEST_F(SequencedObjectTest, RefPtrProvidesWeakPointersBeforeConstruction) {
  SequencedWeakPtr<TestObject> swp;

  {
    // Test that refptr constructs on the proper thread with args.
    SequencedScopedRefPtr<TestObject> test_ref(
        base::SequencedTaskRunnerHandle::Get(), 123);

    // Get a SWP before constructing the object, and use it.
    // swp = test_ref.GetSequencedWeakPtr();
    swp = SequencedWeakPtr<TestObject>(test_ref);
    const int value = 456;
    swp.BindOnce(&TestObject::SetValue).Run(value);
    // Run construction and the callback.
    scoped_task_environment_.RunUntilIdle();

    // TODO(liberato): verify that the copy and move constructors work.

    TestObject* test = test_ref.GetRawPointerForTesting();
    ASSERT_EQ(test->value_, value);
  }

  // Destruction should be posted, but should not have happened yet, since
  // test_ref went out of scope.

  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);
  // This should actually invalidate it.
  scoped_task_environment_.RunUntilIdle();
  ASSERT_EQ(swp.GetRawPointerForTesting(), nullptr);
}

TEST_F(SequencedObjectTest, RefPtrProvidesWeakPointersAfterConstruction) {
  // Test that refptr constructs on the proper thread with args, and run the
  // thread so that construction is complete.
  SequencedWeakPtr<TestObject> swp;
  {
    SequencedScopedRefPtr<TestObject> test_ref(
        base::SequencedTaskRunnerHandle::Get(), 123);
    scoped_task_environment_.RunUntilIdle();
    TestObject* test = test_ref.GetRawPointerForTesting();
    ASSERT_NE(test, nullptr);

    // Get a SWP and use it to bind a callback.  Verify that it posts to the
    // proper sequence.
    swp = SequencedWeakPtr<TestObject>(test_ref);
    const int value = 456;
    swp.BindOnce(&TestObject::SetValue).Run(value);
    ASSERT_NE(test->value_, value);
    scoped_task_environment_.RunUntilIdle();
    ASSERT_EQ(test->value_, value);
  }

  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);
  // This should actually invalidate it.
  scoped_task_environment_.RunUntilIdle();
  ASSERT_EQ(swp.GetRawPointerForTesting(), nullptr);
}

TEST_F(SequencedObjectTest, RefPtrsCanBeCopiedMovedAndReset) {
  // Verify that an object isn't destroyed until the last refptr is.
  SequencedScopedRefPtr<TestObject> test_ref_1(
      base::SequencedTaskRunnerHandle::Get(), 123);
  auto swp = SequencedWeakPtr<TestObject>(test_ref_1);
  SequencedScopedRefPtr<TestObject> test_ref_2 = test_ref_1;

  // Resetting one of the refs shouldn't invalidate the weak ptr.  Note that we
  // do this before construction, just to test that the original ref isn't
  // special in some way before construction completes.
  test_ref_1.reset();
  ASSERT_FALSE((bool)test_ref_1);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);

  // Move-construction should also preserve ownership.
  SequencedScopedRefPtr<TestObject> test_ref_3(std::move(test_ref_2));
  ASSERT_FALSE((bool)test_ref_2);
  ASSERT_TRUE((bool)test_ref_3);
  ASSERT_NE(test_ref_3.GetRawPointerForTesting(), nullptr);
  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);
  scoped_task_environment_.RunUntilIdle();

  // Move-assignment should work.
  SequencedScopedRefPtr<TestObject> test_ref_4;  // Do not assign here!
  test_ref_4 = std::move(test_ref_3);
  ASSERT_FALSE((bool)test_ref_3);
  ASSERT_TRUE((bool)test_ref_4);
  ASSERT_NE(test_ref_4.GetRawPointerForTesting(), nullptr);
  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);
  scoped_task_environment_.RunUntilIdle();

  // Resetting the remaining ref ptr should free the object.
  test_ref_4.reset();
  scoped_task_environment_.RunUntilIdle();
  ASSERT_EQ(swp.GetRawPointerForTesting(), nullptr);
}

TEST_F(SequencedObjectTest, UniquePtrsCanBeCopiedMovedAndReset) {
  const int value = 123;
  // Construct with an argument, and verify that it works.
  SequencedUniquePtr<TestObject> test_unique(
      base::SequencedTaskRunnerHandle::Get(), value);
  ASSERT_TRUE((bool)test_unique);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_EQ(test_unique.GetRawPointerForTesting()->value_, value);

  // Verify that a SWP is valid.
  auto swp = SequencedWeakPtr<TestObject>(test_unique);
  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);

  // Move |test_unique| into another ptr.  |test_unique_2| should point to the
  // underlying object, and RunUntilIdle shouldn't invalidate |swp|.
  SequencedUniquePtr<TestObject> test_unique_2(std::move(test_unique));
  ASSERT_FALSE((bool)test_unique);
  ASSERT_TRUE((bool)test_unique_2);
  ASSERT_NE(test_unique_2.GetRawPointerForTesting(), nullptr);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);

  // Assignment-move |test_unique_2| into another ptr.  Remember that assignment
  // as an initialization uses the move constructor, not move-assignment.
  SequencedUniquePtr<TestObject> test_unique_3;
  test_unique_3 = std::move(test_unique_2);
  ASSERT_FALSE((bool)test_unique_2);
  ASSERT_TRUE((bool)test_unique_3);
  ASSERT_NE(test_unique_3.GetRawPointerForTesting(), nullptr);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);

  // Resetting |test_unique_3| should post destruction, but not invalidate the
  // swp until we RunUntilIdle.
  test_unique_3.reset();
  ASSERT_FALSE((bool)test_unique_3);
  ASSERT_NE(swp.GetRawPointerForTesting(), nullptr);
  scoped_task_environment_.RunUntilIdle();
  ASSERT_EQ(swp.GetRawPointerForTesting(), nullptr);
}

TEST_F(SequencedObjectTest, WeakPtrCastFromDerivedObjectWorks) {
  // Test casting of various types.
  SequencedUniquePtr<DerivedObject> derived_unique(
      base::SequencedTaskRunnerHandle::Get());

  SequencedWeakPtr<DerivedObject> swp_derived(derived_unique);
  SequencedWeakPtr<TestObject> swp_test(derived_unique);
  SequencedWeakPtr<TestObject> swp_test_2(swp_derived);
  swp_test = swp_test_2;
  swp_test = swp_derived;
  SequencedUniquePtr<TestObject> test_unique(std::move(derived_unique));

  // All of these should be valid once the object is constructed.
  scoped_task_environment_.RunUntilIdle();
  ASSERT_FALSE((bool)derived_unique);  // moved, above.
  ASSERT_TRUE(swp_derived.GetRawPointerForTesting());
  ASSERT_TRUE(swp_test.GetRawPointerForTesting());
  ASSERT_TRUE(swp_test_2.GetRawPointerForTesting());
  ASSERT_TRUE((bool)test_unique);

  test_unique.reset();
  scoped_task_environment_.RunUntilIdle();
  ASSERT_FALSE(swp_derived.GetRawPointerForTesting());
  ASSERT_FALSE(swp_test.GetRawPointerForTesting());
  ASSERT_FALSE(swp_test_2.GetRawPointerForTesting());
}

TEST_F(SequencedObjectTest, ScopedRefPtrCastDerivedObjectWorks) {
  SequencedScopedRefPtr<DerivedObject> derived_refptr(
      base::SequencedTaskRunnerHandle::Get());

  SequencedWeakPtr<DerivedObject> swp_derived(derived_refptr);
  SequencedWeakPtr<TestObject> swp_test(derived_refptr);
  SequencedWeakPtr<TestObject> swp_test_2(swp_derived);
  swp_test = swp_test_2;
  swp_test = swp_derived;
  SequencedScopedRefPtr<TestObject> test_refptr_1(derived_refptr);
  SequencedScopedRefPtr<TestObject> test_refptr_2(std::move(derived_refptr));

  // All of these should be valid once the object is constructed.
  scoped_task_environment_.RunUntilIdle();
  ASSERT_FALSE((bool)derived_refptr);  // moved, above.
  ASSERT_TRUE(swp_derived.GetRawPointerForTesting());
  ASSERT_TRUE(swp_test.GetRawPointerForTesting());
  ASSERT_TRUE(swp_test_2.GetRawPointerForTesting());
  ASSERT_TRUE((bool)test_refptr_1);
  ASSERT_TRUE((bool)test_refptr_2);

  // Resetting |test_refptr_1| shouldn't matter.
  test_refptr_1.reset();
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(swp_derived.GetRawPointerForTesting());
  ASSERT_TRUE(swp_test.GetRawPointerForTesting());
  ASSERT_TRUE(swp_test_2.GetRawPointerForTesting());
  ASSERT_FALSE((bool)test_refptr_1);
  ASSERT_TRUE((bool)test_refptr_2);

  // |test_refptr_2| has the only remaining ref.
  test_refptr_2.reset();
  scoped_task_environment_.RunUntilIdle();
  ASSERT_FALSE(swp_derived.GetRawPointerForTesting());
  ASSERT_FALSE(swp_test.GetRawPointerForTesting());
  ASSERT_FALSE(swp_test_2.GetRawPointerForTesting());
  ASSERT_FALSE((bool)test_refptr_1);
  ASSERT_FALSE((bool)test_refptr_2);
}

}  // namespace media
