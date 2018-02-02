// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/promise/promise.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;
using testing::ElementsAreArray;

namespace base {

class PromiseTest : public testing::Test {
 public:
  void RunUntilIdle() { run_loop_.RunUntilIdle(); }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::RunLoop run_loop_;
};

void RecordOrder(std::vector<int>* run_order, int order) {
  run_order->push_back(order);
}

TEST_F(PromiseTest, ToPromiseCallType) {
  static_assert(std::is_same<int, internal::ToPromiseCallType<int>>::value,
                "Integral types should be passed by value");

  static_assert(
      std::is_same<std::unique_ptr<int>,
                   internal::ToPromiseCallType<std::unique_ptr<int>>>::value,
      "Move only types should be passed by value");

  static_assert(std::is_same<const std::string&,
                             internal::ToPromiseCallType<std::string>>::value,
                "Other types should be passed by reference");
}

TEST_F(PromiseTest, UnionOfTypes) {
  static_assert(
      std::is_same<void,
                   typename internal::UnionOfTypes<void, void>::type>::value,
      "Union of identical types should be the type");

  static_assert(
      std::is_same<int, typename internal::UnionOfTypes<int, int>::type>::value,
      "Union of identical types should be the type");

  static_assert(
      std::is_same<Variant<int, bool>,
                   typename internal::UnionOfTypes<int, bool>::type>::value,
      "A union of disjoint types should return a Variant");

  static_assert(
      std::is_same<Variant<int, Void>,
                   typename internal::UnionOfTypes<int, void>::type>::value,
      "A variant can't contain void");

  static_assert(
      std::is_same<Variant<int, float, bool>,
                   typename internal::UnionOfTypes<Variant<int, float>,
                                                   bool>::type>::value,
      "A union of a variant and a disjoint type should be a variant containing "
      "it.");

  static_assert(
      std::is_same<Variant<int, float>,
                   typename internal::UnionOfTypes<Variant<int, float>,
                                                   int>::type>::value,
      "A union of a variant and a type already in it should be the variant.");
}

TEST_F(PromiseTest, UnionOfVariants) {
  static_assert(
      std::is_same<Variant<int, float, bool, char>,
                   typename internal::UnionOfVariants<
                       Variant<int, float>, Variant<bool, char>>::type>::value,
      "A union of two disjoint variants should include all types.");

  static_assert(
      std::is_same<Variant<int, float, bool>,
                   typename internal::UnionOfVariants<
                       Variant<int, float>, Variant<bool, int>>::type>::value,
      "Result should not include duplicates.");

  static_assert(
      std::is_same<Variant<int, float>,
                   typename internal::UnionOfVariants<
                       Variant<int, float>, Variant<float, int>>::type>::value,
      "Result should not include duplicates.");
}

TEST_F(PromiseTest, UnionOfVarArgTypes) {
  static_assert(
      std::is_same<
          Variant<int, float, bool>,
          typename internal::UnionOfVarArgTypes<int, float, bool>::type>::value,
      "Result construct a variant with the same arguments if all are unique.");

  static_assert(
      std::is_same<int, typename internal::UnionOfVarArgTypes<
                            int, int, int>::type>::value,
      "Duplicates should be removed. and if only one type is left then a "
      "variant should not be used..");

  static_assert(
      std::is_same<
          void, typename internal::UnionOfVarArgTypes<void, void>::type>::value,
      "If void is the only type then that should be the result.");

  static_assert(
      std::is_same<Variant<int, Void>, typename internal::UnionOfVarArgTypes<
                                           int, void>::type>::value,
      "Void rather than void must be used in a variant..");
}

TEST_F(PromiseTest, ResolveThen) {
  ManualPromiseResolver<int> p;
  p.GetResolveCallback().Run(123);

  p.promise().Then(FROM_HERE, BindOnce(
                                  [](base::RunLoop* run_loop, int result) {
                                    EXPECT_EQ(123, result);
                                    run_loop->Quit();
                                  },
                                  &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, RejectThen) {
  ManualPromiseResolver<int, std::string> p;
  p.GetRejectCallback().Run(std::string("Oh no!"));

  p.promise().Then(
      FROM_HERE,
      BindOnce(
          [](base::RunLoop* run_loop, int result) {
            run_loop->Quit();
            FAIL() << "We shouldn't get here, the promise was rejected!";
          },
          &run_loop_),
      BindOnce(
          [](base::RunLoop* run_loop, const std::string& err) {
            run_loop->Quit();
            EXPECT_EQ("Oh no!", err);
          },
          &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, ResolveThenType2) {
  Promises::Resolve(123).Then(FROM_HERE,
                              BindOnce(
                                  [](base::RunLoop* run_loop, int result) {
                                    EXPECT_EQ(123, result);
                                    run_loop->Quit();
                                  },
                                  &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, RejectAndReReject) {
  ManualPromiseResolver<int, std::string> p;
  p.GetRejectCallback().Run(std::string("Oh no!"));

  p.promise()
      .Catch(FROM_HERE,
             BindOnce([](const std::string& err) -> PromiseResult<void, int> {
               EXPECT_EQ("Oh no!", err);
               // Re-Reject with -1 this time.
               return -1;
             }))
      .Catch(FROM_HERE, BindOnce(
                            [](base::RunLoop* run_loop,
                               base::Variant<std::string, int> err) {
                              EXPECT_EQ(-1, base::Get<int>(&err));
                              run_loop->Quit();
                            },
                            &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, ResolveThenVoidFunction) {
  ManualPromiseResolver<int> p;
  p.Resolve(123);

  // You don't have to use the resolve (or reject) arguments from the previous
  // promise.
  p.promise().Then(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, ResolveThenStdTupleUnpack) {
  Promises::Resolve(std::make_tuple(10, std::string("Hi")))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int a, std::string b) {
                             EXPECT_EQ(10, a);
                             EXPECT_EQ("Hi", b);
                             run_loop->Quit();
                           },
                           &run_loop_));
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectThenType2) {
  Promises::Reject<int, std::string>("Oh no!").Then(
      FROM_HERE,
      BindOnce(
          [](base::RunLoop* run_loop, int result) {
            run_loop->Quit();
            FAIL() << "We shouldn't get here, the promise was rejected!";
          },
          &run_loop_),
      BindOnce(
          [](base::RunLoop* run_loop, const std::string& err) {
            run_loop->Quit();
            EXPECT_EQ("Oh no!", err);
          },
          &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, ResolveAfterThen) {
  ManualPromiseResolver<int> p;
  p.promise().Then(FROM_HERE, BindOnce(
                                  [](base::RunLoop* run_loop, int result) {
                                    EXPECT_EQ(123, result);
                                    run_loop->Quit();
                                  },
                                  &run_loop_));

  p.Resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectOutsidePriomiseAfterThen) {
  ManualPromiseResolver<int> p;

  p.promise().Then(
      FROM_HERE,
      BindOnce(
          [](base::RunLoop* run_loop, int result) {
            run_loop->Quit();
            FAIL() << "We shouldn't get here, the promise was rejected!";
          },
          &run_loop_),
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  p.Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, ThenChain) {
  ManualPromiseResolver<std::vector<size_t>> p;
  p.promise()
      .Then(FROM_HERE, BindOnce([](std::vector<size_t> result) {
              result.push_back(result.size());
              return result;
            }))
      .Then(FROM_HERE, BindOnce([](std::vector<size_t> result) {
              result.push_back(result.size());
              return result;
            }))
      .Then(FROM_HERE, BindOnce([](std::vector<size_t> result) {
              result.push_back(result.size());
              return result;
            }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, std::vector<size_t> result) {
                  EXPECT_THAT(result, ElementsAre(0u, 1u, 2u, 3u));
                  run_loop->Quit();
                },
                &run_loop_));

  p.Resolve(std::vector<size_t>{0});
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainDefaultVoid) {
  ManualPromiseResolver<std::vector<size_t>> p;
  p.promise()
      .Then(FROM_HERE, BindOnce([](std::vector<size_t> result) {
              result.push_back(result.size());
              return result;
            }))
      .Then(FROM_HERE, BindOnce([](std::vector<size_t> result) {
              result.push_back(result.size());
              return result;
            }))
      .Then(FROM_HERE, BindOnce([](std::vector<size_t> result)
                                    -> PromiseResult<std::vector<size_t>> {
              return Rejected<void>();
            }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, std::vector<size_t> result) {
                  FAIL() << "We shouldn't get here, the promise was rejected!";
                  run_loop->Quit();
                },
                &run_loop_),
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop_));

  p.Resolve(std::vector<size_t>{0});
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainStdString) {
  ManualPromiseResolver<int, std::string> p;
  p.promise()
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE,
            BindOnce([](int result) -> PromiseResult<int, std::string> {
              return std::string("Oh no!");
            }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, int result) {
                  FAIL() << "We shouldn't get here, the promise was rejected!";
                  run_loop->Quit();
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop, const std::string& err) {
                  EXPECT_EQ("Oh no!", err);
                  run_loop->Quit();
                },
                &run_loop_));

  p.Resolve(1);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainMultipleRejectTypes) {
  ManualPromiseResolver<int, bool> p;
  p.promise()
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE,
            BindOnce([](int result) -> PromiseResult<int, std::string> {
              return std::string("Oh no!");
            }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, int result) {
                  FAIL() << "We shouldn't get here, the promise was rejected!";
                  run_loop->Quit();
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop,
                   const Variant<bool, std::string>& err) {
                  EXPECT_EQ("Oh no!", Get<std::string>(&err));
                  run_loop->Quit();
                },
                &run_loop_));

  p.Resolve(1);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainMultipleRejectTypesThensSkipped) {
  ManualPromiseResolver<int, bool> p;
  p.promise()
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE,
            BindOnce([](int result) -> PromiseResult<int, std::string> {
              return std::string("Oh no!");
            }))
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, int result) {
                  FAIL() << "We shouldn't get here, the promise was rejected!";
                  run_loop->Quit();
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop,
                   const Variant<bool, std::string>& err) {
                  EXPECT_EQ("Oh no!", Get<std::string>(&err));
                  run_loop->Quit();
                },
                &run_loop_));

  p.Resolve(1);
  run_loop_.Run();
}

TEST_F(PromiseTest, ThenChainVariousReturnTypes) {
  ManualPromiseResolver<void> p;
  p.promise()
      .Then(FROM_HERE, BindOnce([]() { return 5; }))
      .Then(FROM_HERE, BindOnce([](int result) {
              EXPECT_EQ(5, result);
              return std::string("Hello");
            }))
      .Then(FROM_HERE, BindOnce([](std::string result) {
              EXPECT_EQ("Hello", result);
              return true;
            }))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, bool result) {
                             EXPECT_TRUE(result);
                             run_loop->Quit();
                           },
                           &run_loop_));

  p.GetResolveCallback().Run();
  run_loop_.Run();
}

TEST_F(PromiseTest, CurriedVoidPromise) {
  // We're not allowed to call Run multiple times so we need two run loops.
  base::RunLoop run_loop2;
  Promise<void> p = Promises::Resolve();

  ManualPromiseResolver<void> promise_resolver;
  p.Then(FROM_HERE, BindOnce(
                        [](ManualPromiseResolver<void>* promise_resolver) {
                          return promise_resolver->promise();
                        },
                        &promise_resolver))
      .Then(FROM_HERE,
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop2));
  run_loop_.RunUntilIdle();

  promise_resolver.Resolve();
  run_loop2.Run();
}

TEST_F(PromiseTest, CurriedIntPromise) {
  // We're not allowed to call Run multiple times so we need two run loops.
  base::RunLoop run_loop2;
  Promise<int> p = Promises::Resolve(1000);

  ManualPromiseResolver<int> promise_resolver;
  p.Then(FROM_HERE,
         BindOnce(
             [](ManualPromiseResolver<int>* promise_resolver, int result) {
               EXPECT_EQ(1000, result);
               return promise_resolver->promise();
             },
             &promise_resolver))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(123, result);
                             run_loop->Quit();
                           },
                           &run_loop2));
  run_loop_.RunUntilIdle();

  promise_resolver.Resolve(123);
  run_loop2.Run();
}

TEST_F(PromiseTest, PromiseResultReturningAPromise) {
  // We're not allowed to call Run multiple times so we need two run loops.
  base::RunLoop run_loop2;
  Promise<int> p = Promises::Resolve(1000);

  ManualPromiseResolver<int> promise_resolver;
  p.Then(FROM_HERE, BindOnce(
                        [](ManualPromiseResolver<int>* promise_resolver,
                           int result) -> PromiseResult<int> {
                          EXPECT_EQ(1000, result);
                          return promise_resolver->promise();
                        },
                        &promise_resolver))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(123, result);
                             run_loop->Quit();
                           },
                           &run_loop2));
  run_loop_.RunUntilIdle();

  promise_resolver.Resolve(123);
  run_loop2.Run();
}

TEST_F(PromiseTest, ResolveToDisambiguateThenReturnValue) {
  ManualPromiseResolver<void> p;
  p.Resolve();
  p.promise()
      .Then(FROM_HERE,
            BindOnce([]() -> PromiseResult<Value> { return Value("Hello."); }))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, Value result) {
                             EXPECT_EQ("Hello.", result.GetString());
                             run_loop->Quit();
                           },
                           &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, RejectedToDisambiguateThenReturnValue) {
  ManualPromiseResolver<int, int> p;
  p.Resolve();
  p.promise()
      .Then(FROM_HERE, BindOnce([]() -> PromiseResult<int, int> {
              return Rejected<int>{123};
            }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, int result) {
                  run_loop->Quit();
                  FAIL() << "We shouldn't get here, the promise was rejected!";
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop, int err) {
                  run_loop->Quit();
                  EXPECT_EQ(123, err);
                },
                &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, NestedPromises) {
  ManualPromiseResolver<int> p;
  p.Resolve(100);
  p.promise()
      .Then(FROM_HERE, BindOnce([](int result) {
              ManualPromiseResolver<int> p2;
              p2.Resolve(200);
              return p2.promise().Then(
                  FROM_HERE, BindOnce([](int result) {
                    ManualPromiseResolver<int> p3;
                    p3.Resolve(300);
                    return p3.promise().Then(
                        FROM_HERE, BindOnce([](int result) { return result; }));
                  }));
            }))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(300, result);
                             run_loop->Quit();
                           },
                           &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, Catch) {
  ManualPromiseResolver<int, std::string> p;
  p.promise()
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Catch(FROM_HERE,
             BindOnce(
                 [](base::RunLoop* run_loop, const std::string& err) {
                   EXPECT_EQ("Whoops!", err);
                   run_loop->Quit();
                 },
                 &run_loop_));

  p.Reject("Whoops!");
  run_loop_.Run();
}

TEST_F(PromiseTest, BranchedThenChainExecutionOrder) {
  std::vector<int> run_order;

  ManualPromiseResolver<void> promise_a;
  Promise<void> promise_b =
      promise_a.promise()
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 0))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 1));

  Promise<void> promise_c =
      promise_a.promise()
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 2))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 3));

  Promise<void> promise_d =
      promise_a.promise()
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 4))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 5));

  promise_d.Finally(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  promise_a.Resolve();
  run_loop_.Run();

  EXPECT_THAT(run_order, ElementsAre(0, 2, 4, 1, 3, 5));
}

TEST_F(PromiseTest, BranchedThenChainWithCatchExecutionOrder) {
  std::vector<int> run_order;

  ManualPromiseResolver<void> promise_a;
  Promise<void> promise_b =
      promise_a.promise()
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 0))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 1))
          .Catch(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 2));

  Promise<void> promise_c =
      promise_a.promise()
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 3))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 4))
          .Catch(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 5));

  Promise<void> promise_d =
      promise_a.promise()
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 6))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 7))
          .Catch(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 8));

  promise_d.Finally(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  promise_a.Reject();
  run_loop_.Run();

  EXPECT_THAT(run_order, ElementsAre(2, 5, 8));
}

TEST_F(PromiseTest, CatchRejectInThenChain) {
  ManualPromiseResolver<int> p;
  p.promise()
      .Then(FROM_HERE,
            BindOnce([](int result) -> PromiseResult<int, std::string> {
              return std::string("Whoops!");
            }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Catch(FROM_HERE, BindOnce(
                            [](base::RunLoop* run_loop,
                               const Variant<Void, std::string>& err) {
                              EXPECT_EQ("Whoops!", Get<std::string>(&err));
                              run_loop->Quit();
                            },
                            &run_loop_));

  p.Resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, CatchThenVoid) {
  ManualPromiseResolver<int> p;
  p.promise()
      .Catch(FROM_HERE, BindOnce([]() { return 123; }))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(123, result);
                             run_loop->Quit();
                           },
                           &run_loop_));

  p.Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, CatchThenInt) {
  ManualPromiseResolver<int, int> p;
  p.promise()
      .Catch(FROM_HERE, BindOnce([](int err) { return err + 1; }))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(124, result);
                             run_loop->Quit();
                           },
                           &run_loop_));

  p.Reject(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, SettledTaskFinally) {
  int result = 0;
  ManualPromiseResolver<int> p;
  p.Resolve(123);

  p.promise()
      .Then(FROM_HERE,
            BindOnce([](int* result, int value) { *result = value; }, &result))
      .Finally(FROM_HERE, BindOnce(
                              [](base::RunLoop* run_loop, int* result) {
                                EXPECT_EQ(123, *result);
                                run_loop->Quit();
                              },
                              &run_loop_, &result));

  run_loop_.Run();
}

TEST_F(PromiseTest, ResolveFinally) {
  int result = 0;
  ManualPromiseResolver<int> p;
  p.promise().Then(
      FROM_HERE,
      BindOnce([](int* result, int value) { *result = value; }, &result));
  p.promise().Finally(FROM_HERE, BindOnce(
                                     [](base::RunLoop* run_loop, int* result) {
                                       EXPECT_EQ(123, *result);
                                       run_loop->Quit();
                                     },
                                     &run_loop_, &result));
  p.Resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectFinally) {
  int result = 0;
  ManualPromiseResolver<int> p;
  p.promise().Then(
      FROM_HERE,
      BindOnce([](int* result, int value) { *result = value; }, &result),
      BindOnce([](int* result) { *result = -1; }, &result));
  p.promise().Finally(FROM_HERE, BindOnce(
                                     [](base::RunLoop* run_loop, int* result) {
                                       EXPECT_EQ(-1, *result);
                                       run_loop->Quit();
                                     },
                                     &run_loop_, &result));
  p.Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, CatchThenFinally) {
  std::vector<std::string> log;
  ManualPromiseResolver<void, std::string> p1;
  Promise<void, std::string> p2 =
      p1.promise()
          .Then(FROM_HERE, BindOnce(
                               [](std::vector<std::string>* log) {
                                 log->push_back("Then #1");
                               },
                               &log))
          .Then(FROM_HERE, BindOnce(
                               [](std::vector<std::string>* log)
                                   -> PromiseResult<void, std::string> {
                                 log->push_back("Then #2 (reject)");
                                 return std::string("Whoops!");
                               },
                               &log))
          .Then(FROM_HERE, BindOnce(
                               [](std::vector<std::string>* log) {
                                 log->push_back("Then #3");
                               },
                               &log))
          .Then(FROM_HERE, BindOnce(
                               [](std::vector<std::string>* log) {
                                 log->push_back("Then #4");
                               },
                               &log))
          .Catch(FROM_HERE,
                 BindOnce(
                     [](std::vector<std::string>* log, const std::string& err) {
                       log->push_back("Caught " + err);
                     },
                     &log));

  p2.Finally(FROM_HERE,
             BindOnce(
                 [](std::vector<std::string>* log, base::RunLoop* run_loop) {
                   log->push_back("Finally");
                   run_loop->Quit();
                 },
                 &log, &run_loop_));
  p2.Then(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Then #5"); },
               &log));
  p2.Then(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Then #6"); },
               &log));

  p1.Resolve();
  run_loop_.Run();

  std::vector<std::string> expected_log = {"Then #1",        "Then #2 (reject)",
                                           "Caught Whoops!", "Then #5",
                                           "Then #6",        "Finally"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, ThenChainsWithFinally) {
  std::vector<std::string> log;
  ManualPromiseResolver<void> p;
  p.promise().Finally(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Finally"); },
               &log));

  p.promise()
      .Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then A1");
                           },
                           &log))
      .Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then A2");
                           },
                           &log))
      .Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then A3");
                           },
                           &log));

  p.promise()
      .Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then B1");
                           },
                           &log))
      .Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then B2");
                           },
                           &log))
      .Then(FROM_HERE,
            BindOnce(
                [](std::vector<std::string>* log, base::RunLoop* run_loop) {
                  log->push_back("Then B3");
                  run_loop->Quit();
                },
                &log, &run_loop_));

  p.Resolve();
  run_loop_.Run();

  std::vector<std::string> expected_log = {"Then A1", "Then B1", "Then A2",
                                           "Then B2", "Finally", "Then A3",
                                           "Then B3"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, Cancel) {
  std::vector<std::string> log;
  ManualPromiseResolver<void, std::string> mpr;
  Promise<void, std::string> p1 = mpr.promise();
  Promise<void, std::string> p2 = p1.Then(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Then #1"); },
               &log));
  p2.Then(FROM_HERE, BindOnce(
                         [](std::vector<std::string>* log)
                             -> PromiseResult<void, std::string> {
                           log->push_back("Then #2 (reject)");
                           return std::string("Whoops!");
                         },
                         &log))
      .Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then #3");
                           },
                           &log))
      .Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then #4");
                           },
                           &log))
      .Catch(FROM_HERE,
             BindOnce(
                 [](std::vector<std::string>* log, const std::string& err) {
                   log->push_back("Caught " + err);
                 },
                 &log));

  p2.Finally(FROM_HERE,
             BindOnce(
                 [](std::vector<std::string>* log, base::RunLoop* run_loop) {
                   log->push_back("Finally");
                   run_loop->Quit();
                 },
                 &log, &run_loop_));
  p2.Then(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Then #5"); },
               &log));
  p2.Then(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Then #6"); },
               &log));

  p2.Cancel();

  mpr.Resolve();
  run_loop_.RunUntilIdle();

  EXPECT_TRUE(log.empty());
}

namespace {
struct Cancelable {
  Cancelable() : weak_ptr_factory(this) {}

  void LogTask(std::vector<std::string>* log, std::string value) {
    log->push_back(value);
  }

  base::WeakPtrFactory<Cancelable> weak_ptr_factory;
};
}  // namespace

TEST_F(PromiseTest, CancelViaWeakPtr) {
  std::vector<std::string> log;
  ManualPromiseResolver<void, std::string> mpr;
  Promise<void, std::string> p1 = mpr.promise();
  {
    Cancelable cancelable;
    Promise<void, std::string> p2 =
        p1.Then(FROM_HERE, BindOnce(&Cancelable::LogTask,
                                    cancelable.weak_ptr_factory.GetWeakPtr(),
                                    &log, "Then #1"));
    p2.Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log)
                               -> PromiseResult<void, std::string> {
                             log->push_back("Then #2 (reject)");
                             return std::string("Whoops!");
                           },
                           &log))
        .Then(FROM_HERE, BindOnce(
                             [](std::vector<std::string>* log) {
                               log->push_back("Then #3");
                             },
                             &log))
        .Then(FROM_HERE, BindOnce(
                             [](std::vector<std::string>* log) {
                               log->push_back("Then #4");
                             },
                             &log))
        .Catch(FROM_HERE,
               BindOnce(
                   [](std::vector<std::string>* log, const std::string& err) {
                     log->push_back("Caught " + err);
                   },
                   &log));

    p2.Finally(FROM_HERE,
               BindOnce(
                   [](std::vector<std::string>* log, base::RunLoop* run_loop) {
                     log->push_back("Finally");
                     run_loop->Quit();
                   },
                   &log, &run_loop_));
    p2.Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then #5");
                           },
                           &log));
    p2.Then(FROM_HERE, BindOnce(
                           [](std::vector<std::string>* log) {
                             log->push_back("Then #6");
                           },
                           &log));
  }

  mpr.Resolve();
  run_loop_.RunUntilIdle();

  EXPECT_TRUE(log.empty());
}

TEST_F(PromiseTest, RaceReject) {
  ManualPromiseResolver<float, std::string> p1;
  ManualPromiseResolver<int, std::string> p2;
  ManualPromiseResolver<bool, std::string> p3;

  Promises::Race(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Variant<float, int, bool> result) {
                  run_loop->Quit();
                  FAIL()
                      << "We shouldn't get here, a raced promise was rejected!";
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop, const std::string& err) {
                  run_loop->Quit();
                  EXPECT_EQ("Whoops!", err);
                },
                &run_loop_));

  p3.Reject("Whoops!");
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolve) {
  ManualPromiseResolver<float> p1;
  ManualPromiseResolver<int> p2;
  ManualPromiseResolver<bool> p3;

  Promises::Race(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Variant<float, int, bool> result) {
                  EXPECT_EQ(1337, Get<int>(&result));
                  run_loop->Quit();
                },
                &run_loop_));

  p2.Resolve(1337);
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolveIntVoidVoidInt) {
  ManualPromiseResolver<int> p1;
  ManualPromiseResolver<void> p2;
  ManualPromiseResolver<void> p3;
  ManualPromiseResolver<int> p4;

  Promises::Race(p1.promise(), p2.promise(), p3.promise(), p4.promise())
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Variant<int, Void> result) {
                  EXPECT_TRUE(GetIf<Void>(&result));
                  run_loop->Quit();
                },
                &run_loop_));

  p3.Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceRejectMultipleTypes) {
  ManualPromiseResolver<int> p1;
  ManualPromiseResolver<void, bool> p2;
  ManualPromiseResolver<void, int> p3;
  ManualPromiseResolver<int, std::string> p4;

  Promises::Race(p1.promise(), p2.promise(), p3.promise(), p4.promise())
      .Then(FROM_HERE, BindOnce([](Variant<int, Void> result) {
              FAIL() << "One of the promises was rejected.";
            }),
            BindOnce(
                [](base::RunLoop* run_loop,
                   const Variant<base::Void, bool, int, std::string>& err) {
                  EXPECT_EQ("Oh dear!", Get<std::string>(&err));
                  run_loop->Quit();
                },
                &run_loop_));

  p4.Reject("Oh dear!");
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolveMoveOnlyType) {
  ManualPromiseResolver<std::unique_ptr<float>> p1;
  ManualPromiseResolver<std::unique_ptr<int>> p2;
  ManualPromiseResolver<std::unique_ptr<bool>> p3;

  Promises::Race(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop,
                   Variant<std::unique_ptr<float>, std::unique_ptr<int>,
                           std::unique_ptr<bool>> result) {
                  EXPECT_EQ(1337, *Get<std::unique_ptr<int>>(&result));
                  run_loop->Quit();
                },
                &run_loop_));

  p2.Resolve(std::make_unique<int>(1337));
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolveIntContainer) {
  ManualPromiseResolver<int> mpr1;
  ManualPromiseResolver<int> mpr2;
  ManualPromiseResolver<int> mpr3;
  ManualPromiseResolver<int> mpr4;
  std::vector<Promise<int>> promises{mpr1.promise(), mpr2.promise(),
                                     mpr3.promise(), mpr4.promise()};

  Promises::Race(promises).Then(FROM_HERE,
                                BindOnce(
                                    [](base::RunLoop* run_loop, int result) {
                                      EXPECT_EQ(54321, result);
                                      run_loop->Quit();
                                    },
                                    &run_loop_));

  mpr3.Resolve(54321);
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceRejectVoidContainer) {
  ManualPromiseResolver<void> mpr1;
  ManualPromiseResolver<void> mpr2;
  ManualPromiseResolver<void> mpr3;
  ManualPromiseResolver<void> mpr4;
  std::vector<Promise<void>> promises{mpr1.promise(), mpr2.promise(),
                                      mpr3.promise(), mpr4.promise()};

  Promises::Race(promises).Catch(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  mpr2.Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceVariantContainer) {
  ManualPromiseResolver<int> mpr1;
  ManualPromiseResolver<bool> mpr2;
  ManualPromiseResolver<std::string> mpr3;
  ManualPromiseResolver<bool> mpr4;

  std::vector<Variant<Promise<int>, Promise<bool>, Promise<std::string>>>
      promises;
  promises.push_back(mpr1.promise());
  promises.push_back(mpr2.promise());
  promises.push_back(mpr3.promise());
  promises.push_back(mpr4.promise());

  Promises::Race(promises).Then(
      FROM_HERE,
      BindOnce(
          [](base::RunLoop* run_loop, Variant<int, bool, std::string> result) {
            EXPECT_EQ("Hello", (Get<std::string>(&result)));
            run_loop->Quit();
          },
          &run_loop_));

  mpr3.Resolve("Hello");
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceVariantContainerReject) {
  ManualPromiseResolver<void, std::string> mpr1;
  ManualPromiseResolver<void, std::string> mpr2;
  ManualPromiseResolver<void, int> mpr3;
  ManualPromiseResolver<void, bool> mpr4;

  std::vector<Variant<Promise<void, std::string>, Promise<void, int>,
                      Promise<void, bool>>>
      promises;
  promises.push_back(mpr1.promise());
  promises.push_back(mpr2.promise());
  promises.push_back(mpr3.promise());
  promises.push_back(mpr4.promise());

  Promises::Race(promises).Catch(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop,
                        const Variant<std::string, int, bool>& err) {
                       EXPECT_EQ(-1, Get<int>(&err));
                       run_loop->Quit();
                     },
                     &run_loop_));

  mpr3.Reject(-1);
  run_loop_.Run();
}

TEST_F(PromiseTest, All) {
  ManualPromiseResolver<float> p1;
  ManualPromiseResolver<int> p2;
  ManualPromiseResolver<bool> p3;

  Promise<std::tuple<float, int, bool>> p =
      Promises::All(p1.promise(), p2.promise(), p3.promise());
  p.Then(FROM_HERE,
         BindOnce(
             [](base::RunLoop* run_loop, std::tuple<float, int, bool> result) {
               EXPECT_EQ(1.234f, std::get<0>(result));
               EXPECT_EQ(1234, std::get<1>(result));
               EXPECT_TRUE(std::get<2>(result));
               run_loop->Quit();
             },
             &run_loop_));

  p1.Resolve(1.234f);
  p2.Resolve(1234);
  p3.Resolve(true);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllUnpackTupleTuple) {
  ManualPromiseResolver<float> p1;
  ManualPromiseResolver<int> p2;
  ManualPromiseResolver<bool> p3;

  Promises::All(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, float a, int b, bool c) {
                             EXPECT_EQ(1.234f, a);
                             EXPECT_EQ(1234, b);
                             EXPECT_TRUE(c);
                             run_loop->Quit();
                           },
                           &run_loop_));

  p1.Resolve(1.234f);
  p2.Resolve(1234);
  p3.Resolve(true);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllRejectString) {
  ManualPromiseResolver<float, std::string> p1;
  ManualPromiseResolver<int, std::string> p2;
  ManualPromiseResolver<bool, std::string> p3;

  Promises::All(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE, BindOnce([](std::tuple<float, int, bool> result) {
              FAIL() << "We shouldn't get here, the promise was rejected!";
            }),
            BindOnce(
                [](base::RunLoop* run_loop, const std::string& err) {
                  EXPECT_EQ("Whoops!", err);
                  run_loop->Quit();
                },
                &run_loop_));

  p1.Reject("Whoops!");
  run_loop_.Run();
}

TEST_F(PromiseTest, AllRejectVoid) {
  ManualPromiseResolver<float> p1;
  ManualPromiseResolver<int> p2;
  ManualPromiseResolver<bool> p3;

  Promises::All(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE, BindOnce([](std::tuple<float, int, bool> result) {
              FAIL() << "We shouldn't get here, the promise was rejected!";
            }),
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop_));

  p1.Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntVoid) {
  ManualPromiseResolver<int> p1;
  ManualPromiseResolver<void> p2;

  Promises::All(p1.promise(), p2.promise())
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, std::tuple<int, Void> result) {
                  EXPECT_EQ(1234, std::get<0>(result));
                  run_loop->Quit();
                },
                &run_loop_));

  p1.Resolve(1234);
  p2.Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllMoveOnlyType) {
  ManualPromiseResolver<std::unique_ptr<float>> p1;
  ManualPromiseResolver<std::unique_ptr<int>> p2;
  ManualPromiseResolver<std::unique_ptr<bool>> p3;

  Promises::All(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop,
                   std::tuple<std::unique_ptr<float>, std::unique_ptr<int>,
                              std::unique_ptr<bool>> result) {
                  EXPECT_EQ(1.234f, *std::get<0>(result));
                  EXPECT_EQ(1234, *std::get<1>(result));
                  EXPECT_TRUE(*std::get<2>(result));
                  run_loop->Quit();
                },
                &run_loop_));

  p1.Resolve(std::make_unique<float>(1.234f));
  p2.Resolve(std::make_unique<int>(1234));
  p3.Resolve(std::make_unique<bool>(true));
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntWithVoidThen) {
  ManualPromiseResolver<int> p1;
  ManualPromiseResolver<int> p2;
  ManualPromiseResolver<int> p3;

  // You can choose to ignore the result.
  Promises::All(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE,
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop_));

  p1.Resolve(1);
  p2.Resolve(2);
  p3.Resolve(3);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidWithVoidThen) {
  ManualPromiseResolver<void> p1;
  ManualPromiseResolver<void> p2;
  ManualPromiseResolver<void> p3;

  Promises::All(p1.promise(), p2.promise(), p3.promise())
      .Then(FROM_HERE,
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop_));

  p1.Resolve();
  p2.Resolve();
  p3.Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntContainer) {
  ManualPromiseResolver<int> mpr1;
  ManualPromiseResolver<int> mpr2;
  ManualPromiseResolver<int> mpr3;
  ManualPromiseResolver<int> mpr4;

  std::vector<Promise<int>> promises;
  promises.push_back(mpr1.promise());
  promises.push_back(mpr2.promise());
  promises.push_back(mpr3.promise());
  promises.push_back(mpr4.promise());

  Promises::All(promises).Then(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop, std::vector<int> result) {
                       EXPECT_THAT(result, ElementsAre(10, 20, 30, 40));
                       run_loop->Quit();
                     },
                     &run_loop_));

  mpr1.Resolve(10);
  mpr2.Resolve(20);
  mpr3.Resolve(30);
  mpr4.Resolve(40);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntStringContainerReject) {
  ManualPromiseResolver<int, std::string> mpr1;
  ManualPromiseResolver<int, std::string> mpr2;
  ManualPromiseResolver<int, std::string> mpr3;
  ManualPromiseResolver<int, std::string> mpr4;

  std::vector<Promise<int, std::string>> promises;
  promises.push_back(mpr1.promise());
  promises.push_back(mpr2.promise());
  promises.push_back(mpr3.promise());
  promises.push_back(mpr4.promise());

  Promises::All(promises).Catch(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop, const std::string& err) {
                       EXPECT_EQ("Oh dear", err);
                       run_loop->Quit();
                     },
                     &run_loop_));

  mpr2.Reject("Oh dear");
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidContainer) {
  ManualPromiseResolver<void> mpr1;
  ManualPromiseResolver<void> mpr2;
  ManualPromiseResolver<void> mpr3;
  ManualPromiseResolver<void> mpr4;

  std::vector<Promise<void>> promises;
  promises.push_back(mpr1.promise());
  promises.push_back(mpr2.promise());
  promises.push_back(mpr3.promise());
  promises.push_back(mpr4.promise());

  Promises::All(promises).Then(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop, std::vector<Void> result) {
                       EXPECT_EQ(4u, result.size());
                       run_loop->Quit();
                     },
                     &run_loop_));

  mpr1.Resolve();
  mpr2.Resolve();
  mpr3.Resolve();
  mpr4.Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidIntContainerReject) {
  ManualPromiseResolver<void, int> mpr1;
  ManualPromiseResolver<void, int> mpr2;
  ManualPromiseResolver<void, int> mpr3;
  ManualPromiseResolver<void, int> mpr4;

  std::vector<Promise<void, int>> promises;
  promises.push_back(mpr1.promise());
  promises.push_back(mpr2.promise());
  promises.push_back(mpr3.promise());
  promises.push_back(mpr4.promise());

  Promises::All(promises).Catch(FROM_HERE,
                                BindOnce(
                                    [](base::RunLoop* run_loop, int err) {
                                      EXPECT_EQ(-1, err);
                                      run_loop->Quit();
                                    },
                                    &run_loop_));

  mpr1.Reject(-1);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidContainerReject) {
  ManualPromiseResolver<void> mpr1;
  ManualPromiseResolver<void> mpr2;
  ManualPromiseResolver<void> mpr3;
  ManualPromiseResolver<void> mpr4;

  std::vector<Promise<void>> promises;
  promises.push_back(mpr1.promise());
  promises.push_back(mpr2.promise());
  promises.push_back(mpr3.promise());
  promises.push_back(mpr4.promise());

  Promises::All(promises).Catch(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  mpr4.Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVariantContainer) {
  ManualPromiseResolver<int> p1;
  ManualPromiseResolver<void> p2;
  ManualPromiseResolver<std::string> p3;

  std::vector<Variant<Promise<int>, Promise<void>, Promise<std::string>>>
      promises;
  promises.push_back(p1.promise());
  promises.push_back(p2.promise());
  promises.push_back(p3.promise());

  Promises::All(promises).Then(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop,
                        std::vector<Variant<int, Void, std::string>> result) {
                       EXPECT_EQ(10, (Get<int>(&result[0])));
                       EXPECT_EQ("three", (Get<std::string>(&result[2])));
                       run_loop->Quit();
                     },
                     &run_loop_));

  p1.Resolve(10);
  p2.Resolve();
  p3.Resolve(std::string("three"));
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVariantContainerReject) {
  ManualPromiseResolver<int, int> p1;
  ManualPromiseResolver<void, int> p2;
  ManualPromiseResolver<std::string, int> p3;

  std::vector<
      Variant<Promise<int, int>, Promise<void, int>, Promise<std::string, int>>>
      promises;
  promises.push_back(p1.promise());
  promises.push_back(p2.promise());
  promises.push_back(p3.promise());

  Promises::All(promises).Catch(FROM_HERE,
                                BindOnce(
                                    [](base::RunLoop* run_loop, int err) {
                                      EXPECT_EQ(-1, err);
                                      run_loop->Quit();
                                    },
                                    &run_loop_));

  p2.Reject(-1);
  run_loop_.Run();
}

}  // namespace base
