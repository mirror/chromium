// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/promise.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

template <typename R, typename... Args>
R NeverCalled(Args...) {
  EXPECT_FALSE(true);
}

template <typename Signature = void()>
OnceCallback<Signature> MakeNeverCalled() {
  Signature* never_called = &NeverCalled;
  return BindOnce(never_called);
}

}  // namespace

TEST(PromiseTest, Invalidate) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();

  EXPECT_TRUE(future);
  EXPECT_TRUE(promise);
  std::move(promise).Resolve(FROM_HERE);
  Future<void> future2 = std::move(future).Then(FROM_HERE, BindOnce([] {}));
  EXPECT_FALSE(future);
  EXPECT_FALSE(promise);

  EXPECT_TRUE(future2);
  std::move(future2).Catch(FROM_HERE, BindOnce([] {}));
  EXPECT_FALSE(future2);

  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, ThenAfterResolve_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();
  std::move(promise).Resolve(FROM_HERE);

  bool ran = false;
  std::move(future).Then(FROM_HERE, BindLambdaForTesting([&] {
                           EXPECT_FALSE(ran);
                           ran = true;
                         }));
  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ResolveAfterThen_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();

  bool ran = false;
  std::move(future).Then(FROM_HERE, BindLambdaForTesting([&] {
                           EXPECT_FALSE(ran);
                           ran = true;
                         }));
  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_FALSE(ran);

  std::move(promise).Resolve(FROM_HERE);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ThenAfterReject_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();
  std::move(promise).Reject(FROM_HERE);
  std::move(future).Then(FROM_HERE, MakeNeverCalled<>());
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, RejectAfterThen_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();

  std::move(future).Then(FROM_HERE, MakeNeverCalled<>());
  std::move(promise).Reject(FROM_HERE);
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, ThenAfterResolve_int) {
  MessageLoop message_loop;

  Future<int> future;
  Promise<int> promise;
  std::tie(future, promise) = Contract<int>();
  std::move(promise).Resolve(FROM_HERE, 42);

  bool ran = false;
  std::move(future).Then(FROM_HERE, BindLambdaForTesting([&](int x) {
                           EXPECT_EQ(42, x);
                           EXPECT_FALSE(ran);
                           ran = true;
                         }));
  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ResolveAfterThen_int) {
  MessageLoop message_loop;

  Future<int> future;
  Promise<int> promise;
  std::tie(future, promise) = Contract<int>();

  bool ran = false;
  std::move(future).Then(FROM_HERE, BindLambdaForTesting([&](int x) {
                           EXPECT_EQ(42, x);
                           EXPECT_FALSE(ran);
                           ran = true;
                         }));
  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_FALSE(ran);

  std::move(promise).Resolve(FROM_HERE, 42);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ThenAfterReject_int) {
  MessageLoop message_loop;

  Future<void, int> future;
  Promise<void, int> promise;
  std::tie(future, promise) = Contract<void, int>();

  std::move(promise).Reject(FROM_HERE, 42);
  std::move(future).Then(FROM_HERE, MakeNeverCalled());
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, RejectAfterThen_int) {
  MessageLoop message_loop;

  Future<void, int> future;
  Promise<void, int> promise;
  std::tie(future, promise) = Contract<void, int>();

  std::move(future).Then(FROM_HERE, MakeNeverCalled<>());
  std::move(promise).Reject(FROM_HERE, 42);
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, CatchAfterResolve_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();
  std::move(promise).Resolve(FROM_HERE);
  std::move(future).Catch(FROM_HERE, MakeNeverCalled());
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, ResolveAfterCatch_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();
  std::move(future).Catch(FROM_HERE, MakeNeverCalled<>());
  std::move(promise).Resolve(FROM_HERE);
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, CatchAfterResolve_int) {
  MessageLoop message_loop;

  Future<void, int> future;
  Promise<void, int> promise;
  std::tie(future, promise) = Contract<void, int>();
  std::move(promise).Resolve(FROM_HERE);
  std::move(future).Catch(FROM_HERE, MakeNeverCalled<void(int)>());
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, ResolveAfterCatch_int) {
  MessageLoop message_loop;

  Future<void, int> future;
  Promise<void, int> promise;
  std::tie(future, promise) = Contract<void, int>();
  std::move(future).Catch(FROM_HERE, MakeNeverCalled<void(int)>());
  std::move(promise).Resolve(FROM_HERE);
  RunLoop().RunUntilIdle();
}

TEST(PromiseTest, CatchAfterReject_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();
  std::move(promise).Reject(FROM_HERE);

  bool ran = false;
  std::move(future).Catch(FROM_HERE, BindLambdaForTesting([&] {
                            EXPECT_FALSE(ran);
                            ran = true;
                          }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, RejectAfterCatch_void) {
  MessageLoop message_loop;

  Future<void> future;
  Promise<void> promise;
  std::tie(future, promise) = Contract<void>();

  bool ran = false;
  std::move(future).Catch(FROM_HERE, BindLambdaForTesting([&] {
                            EXPECT_FALSE(ran);
                            ran = true;
                          }));
  std::move(promise).Reject(FROM_HERE);

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, CatchAfterReject_int) {
  MessageLoop message_loop;

  Future<void, int> future;
  Promise<void, int> promise;
  std::tie(future, promise) = Contract<void, int>();
  std::move(promise).Reject(FROM_HERE, 42);

  bool ran = false;
  std::move(future).Catch(FROM_HERE, BindLambdaForTesting([&](int x) {
                            EXPECT_EQ(42, x);
                            EXPECT_FALSE(ran);
                            ran = true;
                          }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, RejectAfterCatch_int) {
  MessageLoop message_loop;

  Future<void, int> future;
  Promise<void, int> promise;
  std::tie(future, promise) = Contract<void, int>();

  bool ran = false;
  std::move(future).Catch(FROM_HERE, BindLambdaForTesting([&](int x) {
                            EXPECT_EQ(42, x);
                            EXPECT_FALSE(ran);
                            ran = true;
                          }));
  std::move(promise).Reject(FROM_HERE, 42);

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ChainToValue) {
  MessageLoop message_loop;

  Future<void, NotReached> future = Resolve(FROM_HERE);

  bool ran = false;
  std::move(future)
      .Then(FROM_HERE, BindOnce([] { return 42; }))
      .Then(FROM_HERE, BindLambdaForTesting([&](int x) {
              EXPECT_EQ(42, x);
              EXPECT_FALSE(ran);
              ran = true;
            }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ChainToResolved) {
  MessageLoop message_loop;

  Future<void, NotReached> future = Resolve(FROM_HERE);

  bool ran = false;
  std::move(future)
      .Then(FROM_HERE, BindOnce([] { return Resolve(FROM_HERE, 42); }))
      .Then(FROM_HERE, BindLambdaForTesting([&](int x) {
              EXPECT_EQ(42, x);
              EXPECT_FALSE(ran);
              ran = true;
            }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, ChainToRejected) {
  MessageLoop message_loop;

  Future<void, NotReached> future = Resolve(FROM_HERE);

  bool ran = false;
  std::move(future)
      .Then(FROM_HERE, BindOnce([] { return Reject(FROM_HERE, 42); }))
      .Catch(FROM_HERE, BindLambdaForTesting([&](int x) {
               EXPECT_EQ(42, x);
               EXPECT_FALSE(ran);
               ran = true;
             }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, RecoverToValue) {
  MessageLoop message_loop;

  Future<NotReached, void> future = Reject(FROM_HERE);

  bool ran = false;
  std::move(future)
      .Catch(FROM_HERE, BindOnce([] { return 42; }))
      .Then(FROM_HERE, BindLambdaForTesting([&](int x) {
              EXPECT_EQ(42, x);
              EXPECT_FALSE(ran);
              ran = true;
            }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, RecoverToResolved) {
  MessageLoop message_loop;

  Future<NotReached, void> future = Reject(FROM_HERE);

  bool ran = false;
  std::move(future)
      .Catch(FROM_HERE, BindOnce([] { return Resolve(FROM_HERE, 42); }))
      .Then(FROM_HERE, BindLambdaForTesting([&](int x) {
              EXPECT_EQ(42, x);
              EXPECT_FALSE(ran);
              ran = true;
            }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

TEST(PromiseTest, RecoverToReject) {
  MessageLoop message_loop;

  Future<NotReached, void> future = Reject(FROM_HERE);

  bool ran = false;
  std::move(future)
      .Catch(FROM_HERE, BindOnce([] { return Reject(FROM_HERE, 42); }))
      .Catch(FROM_HERE, BindLambdaForTesting([&](int x) {
               EXPECT_EQ(42, x);
               EXPECT_FALSE(ran);
               ran = true;
             }));

  EXPECT_FALSE(ran);
  RunLoop().RunUntilIdle();
  EXPECT_TRUE(ran);
}

}  // namespace base
