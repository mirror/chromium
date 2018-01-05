// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"

#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/callback_internal.h"
#include "base/memory/ref_counted.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

void NopInvokeFunc() {}

// White-box testpoints to inject into a Callback<> object for checking
// comparators and emptiness APIs.  Use a BindState that is specialized
// based on a type we declared in the anonymous namespace above to remove any
// chance of colliding with another instantiation and breaking the
// one-definition-rule.
struct FakeBindState : internal::BindStateBase {
  FakeBindState() : BindStateBase(&NopInvokeFunc, &Destroy, &IsCancelled) {}

 private:
  ~FakeBindState() = default;
  static void Destroy(const internal::BindStateBase* self) {
    delete static_cast<const FakeBindState*>(self);
  }
  static bool IsCancelled(const internal::BindStateBase*) {
    return false;
  }
};

namespace {

class CallbackTest : public ::testing::Test {
 public:
  CallbackTest()
      : callback_a_(new FakeBindState()), callback_b_(new FakeBindState()) {}

  ~CallbackTest() override = default;

 protected:
  Callback<void()> callback_a_;
  const Callback<void()> callback_b_;  // Ensure APIs work with const.
};

// Ensure we can create unbound callbacks. We need this to be able to store
// them in class members that can be initialized later.
TEST_F(CallbackTest, DefaultConstruction) {
  Callback<void()> c0;
  Callback<void(int, int)> c1;

  EXPECT_FALSE(c0);
  EXPECT_FALSE(c1);

  EXPECT_TRUE(c0.is_null());
  EXPECT_TRUE(c1.is_null());
}

TEST_F(CallbackTest, NullCallback) {
  Callback<void()> cb = base::null_callback;
  EXPECT_FALSE(cb);

  ASSERT_TRUE(callback_a_);
  callback_a_ = base::null_callback;
  ASSERT_FALSE(callback_a_);
}

TEST_F(CallbackTest, IsNull) {
  EXPECT_TRUE(Closure().is_null());
  EXPECT_FALSE(callback_a_.is_null());
  EXPECT_FALSE(callback_b_.is_null());
}

TEST_F(CallbackTest, Equals) {
  EXPECT_TRUE(callback_a_.Equals(callback_a_));
  EXPECT_FALSE(callback_a_.Equals(callback_b_));
  EXPECT_FALSE(callback_b_.Equals(callback_a_));

  // We should compare based on instance, not type.
  Callback<void()> callback_c(new FakeBindState());
  Callback<void()> callback_a2 = callback_a_;
  EXPECT_TRUE(callback_a_.Equals(callback_a2));
  EXPECT_FALSE(callback_a_.Equals(callback_c));

  // Empty, however, is always equal to empty.
  Callback<void()> empty;
  Callback<void()> empty2;
  EXPECT_TRUE(empty.Equals(empty2));
}

TEST_F(CallbackTest, Reset) {
  // Resetting should bring us back to empty.
  ASSERT_FALSE(callback_a_.is_null());
  ASSERT_FALSE(callback_a_.Equals(Closure()));

  callback_a_.Reset();

  EXPECT_TRUE(callback_a_.is_null());
  EXPECT_TRUE(callback_a_.Equals(Closure()));
}

TEST_F(CallbackTest, Move) {
  // Moving should reset the callback.
  ASSERT_FALSE(callback_a_.is_null());
  ASSERT_FALSE(callback_a_.Equals(Closure()));

  auto tmp = std::move(callback_a_);

  EXPECT_TRUE(callback_a_.is_null());
  EXPECT_TRUE(callback_a_.Equals(Closure()));
}

struct TestForReentrancy {
  TestForReentrancy()
      : cb_already_run(false),
        cb(Bind(&TestForReentrancy::AssertCBIsNull, Unretained(this))) {
  }
  void AssertCBIsNull() {
    ASSERT_TRUE(cb.is_null());
    cb_already_run = true;
  }
  bool cb_already_run;
  Closure cb;
};

TEST_F(CallbackTest, ResetAndReturn) {
  TestForReentrancy tfr;
  ASSERT_FALSE(tfr.cb.is_null());
  ASSERT_FALSE(tfr.cb_already_run);
  ResetAndReturn(&tfr.cb).Run();
  ASSERT_TRUE(tfr.cb.is_null());
  ASSERT_TRUE(tfr.cb_already_run);
}

TEST_F(CallbackTest, NullAfterMoveRun) {
  Closure cb = Bind([] {});
  ASSERT_TRUE(cb);
  std::move(cb).Run();
  ASSERT_FALSE(cb);

  const Closure cb2 = Bind([] {});
  ASSERT_TRUE(cb2);
  std::move(cb2).Run();
  ASSERT_TRUE(cb2);

  OnceClosure cb3 = BindOnce([] {});
  ASSERT_TRUE(cb3);
  std::move(cb3).Run();
  ASSERT_FALSE(cb3);
}

class CallbackOwner : public base::RefCounted<CallbackOwner> {
 public:
  explicit CallbackOwner(bool* deleted) {
    callback_ = Bind(&CallbackOwner::Unused, this);
    deleted_ = deleted;
  }
  void Reset() {
    callback_.Reset();
    // We are deleted here if no-one else had a ref to us.
  }

 private:
  friend class base::RefCounted<CallbackOwner>;
  virtual ~CallbackOwner() {
    *deleted_ = true;
  }
  void Unused() {
    FAIL() << "Should never be called";
  }

  Closure callback_;
  bool* deleted_;
};

TEST_F(CallbackTest, CallbackHasLastRefOnContainingObject) {
  bool deleted = false;
  CallbackOwner* owner = new CallbackOwner(&deleted);
  owner->Reset();
  ASSERT_TRUE(deleted);
}

}  // namespace
}  // namespace base
