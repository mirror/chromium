// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/scoped_blocking_call.h"

#include <memory>

#include "base/macros.h"
#include "base/test/gtest_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

class MockBlockingObserver : public internal::BlockingObserver {
 public:
  MockBlockingObserver() = default;

  MOCK_METHOD1(BlockingScopeEntered, void(BlockingType));
  MOCK_METHOD0(BlockingScopeExited, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockBlockingObserver);
};

class ScopedBlockingCallTest : public testing::Test {
 protected:
  ScopedBlockingCallTest() {
    internal::SetBlockingObserverForCurrentThread(&observer_);
  }

  ~ScopedBlockingCallTest() { internal::ClearBlockingObserverForTesting(); }

  testing::StrictMock<MockBlockingObserver> observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedBlockingCallTest);
};

}  // namespace

TEST_F(ScopedBlockingCallTest, MayBlock) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::MAY_BLOCK));
  ScopedBlockingCall scoped_blocking_call(BlockingType::MAY_BLOCK);
  testing::Mock::VerifyAndClear(&observer_);
  EXPECT_CALL(observer_, BlockingScopeExited());
}

TEST_F(ScopedBlockingCallTest, WillBlock) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::WILL_BLOCK));
  ScopedBlockingCall scoped_blocking_call(BlockingType::WILL_BLOCK);
  testing::Mock::VerifyAndClear(&observer_);
  EXPECT_CALL(observer_, BlockingScopeExited());
}

TEST_F(ScopedBlockingCallTest, MayBlockWillBlock) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::MAY_BLOCK));
  ScopedBlockingCall scoped_blocking_call_a(BlockingType::MAY_BLOCK);
  testing::Mock::VerifyAndClear(&observer_);

  {
    EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::WILL_BLOCK));
    ScopedBlockingCall scoped_blocking_call_b(BlockingType::WILL_BLOCK);
    testing::Mock::VerifyAndClear(&observer_);
  }

  EXPECT_CALL(observer_, BlockingScopeExited());
}

TEST_F(ScopedBlockingCallTest, WillBlockMayBlock) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::WILL_BLOCK));
  ScopedBlockingCall scoped_blocking_call_a(BlockingType::WILL_BLOCK);
  testing::Mock::VerifyAndClear(&observer_);

  { ScopedBlockingCall scoped_blocking_call_b(BlockingType::MAY_BLOCK); }

  EXPECT_CALL(observer_, BlockingScopeExited());
}

TEST_F(ScopedBlockingCallTest, MayBlockMayBlock) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::MAY_BLOCK));
  ScopedBlockingCall scoped_blocking_call_a(BlockingType::MAY_BLOCK);
  testing::Mock::VerifyAndClear(&observer_);

  { ScopedBlockingCall scoped_blocking_call_b(BlockingType::MAY_BLOCK); }

  EXPECT_CALL(observer_, BlockingScopeExited());
}

TEST_F(ScopedBlockingCallTest, WillBlockWillBlock) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::WILL_BLOCK));
  ScopedBlockingCall scoped_blocking_call_a(BlockingType::WILL_BLOCK);
  testing::Mock::VerifyAndClear(&observer_);

  { ScopedBlockingCall scoped_blocking_call_b(BlockingType::WILL_BLOCK); }

  EXPECT_CALL(observer_, BlockingScopeExited());
}

TEST_F(ScopedBlockingCallTest, MayBlockWillBlockTwice) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::MAY_BLOCK));
  ScopedBlockingCall scoped_blocking_call_a(BlockingType::MAY_BLOCK);
  testing::Mock::VerifyAndClear(&observer_);

  {
    EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::WILL_BLOCK));
    ScopedBlockingCall scoped_blocking_call_b(BlockingType::WILL_BLOCK);
    testing::Mock::VerifyAndClear(&observer_);

    {
      ScopedBlockingCall scoped_blocking_call_c(BlockingType::MAY_BLOCK);
      ScopedBlockingCall scoped_blocking_call_d(BlockingType::WILL_BLOCK);
    }
  }

  EXPECT_CALL(observer_, BlockingScopeExited());
}

TEST_F(ScopedBlockingCallTest, InvalidDestructionOrder) {
  EXPECT_CALL(observer_, BlockingScopeEntered(BlockingType::WILL_BLOCK));
  auto scoped_blocking_call_a =
      std::make_unique<ScopedBlockingCall>(BlockingType::WILL_BLOCK);
  auto scoped_blocking_call_b =
      std::make_unique<ScopedBlockingCall>(BlockingType::WILL_BLOCK);

  EXPECT_DCHECK_DEATH({ scoped_blocking_call_a.reset(); });

  EXPECT_CALL(observer_, BlockingScopeExited());
}

}  // namespace base
