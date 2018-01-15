// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/promise.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

TEST(PromiseTest, ThenAfterResolve_Void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();
  std::move(promise).Resolve(FROM_HERE);
  std::move(future).Then(FROM_HERE, BindOnce([] {}));

  bool ran = false;
  auto done = [](bool* ran) {
    EXPECT_FALSE(*ran);
    *ran = true;
    return Resolve(FROM_HERE, 1);
  };

  Future<int> chained = std::move(future).Then(FROM_HERE, BindOnce(done, &ran));
  std::move(chained).Then(FROM_HERE, message_loop.task_runner(),
                          BindOnce([](int x) { EXPECT_EQ(1, x); }));

  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

}  // namespace base
