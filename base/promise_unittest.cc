// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/test/bind_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

TEST(PromiseTest, ResolveFirst) {
  MessageLoop message_loop;

  Future<int> future;
  Promise<int> promise;
  std::tie(future, promise) = Contract<int>();

  bool ran = false;
  std::move(promise).Resolve(FROM_HERE, 42);
  std::move(future).Then(BindLambdaForTesting([&](int x) {
    EXPECT_FALSE(ran);
    EXPECT_EQ(42, x);
    ran = true;
  }));

  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ResolveLater) {
  MessageLoop message_loop;

  Future<int> future;
  Promise<int> promise;
  std::tie(future, promise) = Contract<int>();

  bool ran = false;
  std::move(future).Then(BindLambdaForTesting([&](int x) {
    EXPECT_FALSE(ran);
    EXPECT_EQ(42, x);
    ran = true;
  }));

  RunLoop().RunUntilIdle();
  EXPECT_FALSE(ran);

  std::move(promise).Resolve(FROM_HERE, 42);

  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, PromiseChain) {
  MessageLoop message_loop;

  Future<int> future;
  Promise<int> promise;
  std::tie(future, promise) = Contract<int>();

  bool ran = false;
  Future<double> chained = std::move(future).Chain(
      FROM_HERE, BindOnce([&](int x) { return x * 6.0; }));

  std::move(chained).Then(BindLambdaForTesting([&](double x) {
    EXPECT_EQ(42, x);
    ran = true;
  }));

  RunLoop().RunUntilIdle();
  EXPECT_FALSE(ran);

  std::move(promise).Resolve(FROM_HERE, 7);

  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

}  // namespace base
