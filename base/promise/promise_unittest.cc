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

TEST_F(PromiseTest, ResolveThen) {
  Promise<int> p;
  p.GetResolver()->Resolve(123);

  p.Then(FROM_HERE, BindOnce(
                        [](base::RunLoop* run_loop, int result) {
                          EXPECT_EQ(123, result);
                          run_loop->Quit();
                        },
                        &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, RejectThen) {
  Promise<int, std::string> p;
  p.GetResolver()->Reject("Oh no!");

  p.Then(FROM_HERE,
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
  promise::Resolve(123).Then(FROM_HERE,
                             BindOnce(
                                 [](base::RunLoop* run_loop, int result) {
                                   EXPECT_EQ(123, result);
                                   run_loop->Quit();
                                 },
                                 &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, ResolveThenVoidFunction) {
  Promise<int> p;
  p.GetResolver()->Resolve(123);

  // You don't have to use the resolve (or reject) arguments from the previous
  // promise.
  p.Then(FROM_HERE, BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                             &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, ResolveThenStdTupleUnpack) {
  promise::Resolve(std::make_tuple(10, std::string("Hi")))
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
  promise::Reject<int, std::string>("Oh no!").Then(
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
  Promise<int> p;
  p.Then(FROM_HERE, BindOnce(
                        [](base::RunLoop* run_loop, int result) {
                          EXPECT_EQ(123, result);
                          run_loop->Quit();
                        },
                        &run_loop_));

  p.GetResolver()->Resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectOutsidePriomiseAfterThen) {
  Promise<int> p;

  p.Then(
      FROM_HERE,
      BindOnce(
          [](base::RunLoop* run_loop, int result) {
            run_loop->Quit();
            FAIL() << "We shouldn't get here, the promise was rejected!";
          },
          &run_loop_),
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  p.GetResolver()->Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, ThenChain) {
  Promise<std::vector<size_t>> p;
  p.Then(FROM_HERE, BindOnce([](std::vector<size_t> result) {
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

  p.GetResolver()->Resolve(std::vector<size_t>{0});
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainDefaultVoid) {
  Promise<std::vector<size_t>> p;
  p.Then(FROM_HERE, BindOnce([](std::vector<size_t> result) {
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

  p.GetResolver()->Resolve(std::vector<size_t>{0});
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainStdString) {
  Promise<int, std::string> p;
  p.Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
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

  p.GetResolver()->Resolve(1);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainMultipleRejectTypes) {
  Promise<int, bool> p;
  p.Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
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

  p.GetResolver()->Resolve(1);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectionInThenChainMultipleRejectTypesThensSkipped) {
  Promise<int, bool> p;
  p.Then(FROM_HERE, BindOnce([](int result) { return result + 1; }))
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

  p.GetResolver()->Resolve(1);
  run_loop_.Run();
}

TEST_F(PromiseTest, ThenChainVariousReturnTypes) {
  Promise<void> p;
  p.Then(FROM_HERE, BindOnce([]() { return 5; }))
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

  p.GetResolver()->Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, CurriedVoidPromise) {
  // We're not allowed to call Run multiple times so we need two run loops.
  base::RunLoop run_loop2;
  Promise<void> p = promise::Resolve();

  PromiseResolver<void> promise_resolver;
  p.Then(FROM_HERE, BindOnce(
                        [](PromiseResolver<void>* promise_resolver) {
                          Promise<void> p2;
                          *promise_resolver = *p2.GetResolver();
                          return p2;
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
  Promise<int> p = promise::Resolve(1000);

  PromiseResolver<int> promise_resolver;
  p.Then(FROM_HERE, BindOnce(
                        [](PromiseResolver<int>* promise_resolver, int result) {
                          EXPECT_EQ(1000, result);
                          Promise<int> p2;
                          *promise_resolver = *p2.GetResolver();
                          return p2;
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
  Promise<void> p;
  p.GetResolver()->Resolve();
  p.Then(FROM_HERE,
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
  Promise<int, int> p;
  p.GetResolver()->Resolve();
  p.Then(FROM_HERE, BindOnce([]() -> PromiseResult<int, int> {
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
  Promise<int> p;
  p.GetResolver()->Resolve(100);
  p.Then(FROM_HERE, BindOnce([](int result) {
           Promise<int> p2;
           p2.GetResolver()->Resolve(200);
           return p2.Then(FROM_HERE, BindOnce([](int result) {
                            Promise<int> p3;
                            p3.GetResolver()->Resolve(300);
                            return p3.Then(FROM_HERE, BindOnce([](int result) {
                                             return result;
                                           }));
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
  Promise<int, std::string> p;
  p.Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Catch(FROM_HERE,
             BindOnce(
                 [](base::RunLoop* run_loop, const std::string& err) {
                   EXPECT_EQ("Whoops!", err);
                   run_loop->Quit();
                 },
                 &run_loop_));

  p.GetResolver()->Reject("Whoops!");
  run_loop_.Run();
}

TEST_F(PromiseTest, BranchedThenChainExecutionOrder) {
  std::vector<int> run_order;

  Promise<void> promise_a;
  Promise<void> promise_b =
      promise_a.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 0))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 1));

  Promise<void> promise_c =
      promise_a.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 2))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 3));

  Promise<void> promise_d =
      promise_a.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 4))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 5));

  promise_d.Finally(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  promise_a.GetResolver()->Resolve();
  run_loop_.Run();

  EXPECT_THAT(run_order, ElementsAre(0, 2, 4, 1, 3, 5));
}

TEST_F(PromiseTest, BranchedThenChainWithCatchExecutionOrder) {
  std::vector<int> run_order;

  Promise<void> promise_a;
  Promise<void> promise_b =
      promise_a.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 0))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 1))
          .Catch(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 2));

  Promise<void> promise_c =
      promise_a.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 3))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 4))
          .Catch(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 5));

  Promise<void> promise_d =
      promise_a.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 6))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 7))
          .Catch(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 8));

  promise_d.Finally(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  promise_a.GetResolver()->Reject();
  run_loop_.Run();

  EXPECT_THAT(run_order, ElementsAre(2, 5, 8));
}

TEST_F(PromiseTest, CatchRejectInThenChain) {
  Promise<int> p;
  p.Then(FROM_HERE, BindOnce([](int result) -> PromiseResult<int, std::string> {
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

  p.GetResolver()->Resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, CatchThenVoid) {
  Promise<int> p;
  p.Catch(FROM_HERE, BindOnce([]() { return 123; }))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(123, result);
                             run_loop->Quit();
                           },
                           &run_loop_));

  p.GetResolver()->Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, CatchThenInt) {
  Promise<int, int> p;
  p.Catch(FROM_HERE, BindOnce([](int err) { return err + 1; }))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(124, result);
                             run_loop->Quit();
                           },
                           &run_loop_));

  p.GetResolver()->Reject(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, SettledTaskFinally) {
  int result = 0;
  Promise<int> p;
  p.GetResolver()->Resolve(123);

  p.Then(FROM_HERE,
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
  Promise<int> p;
  p.Then(FROM_HERE,
         BindOnce([](int* result, int value) { *result = value; }, &result));
  p.Finally(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int* result) {
                             EXPECT_EQ(123, *result);
                             run_loop->Quit();
                           },
                           &run_loop_, &result));
  p.GetResolver()->Resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, RejectFinally) {
  int result = 0;
  Promise<int> p;
  p.Then(FROM_HERE,
         BindOnce([](int* result, int value) { *result = value; }, &result),
         BindOnce([](int* result) { *result = -1; }, &result));
  p.Finally(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int* result) {
                             EXPECT_EQ(-1, *result);
                             run_loop->Quit();
                           },
                           &run_loop_, &result));
  p.GetResolver()->Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, CatchThenFinally) {
  std::vector<std::string> log;
  Promise<void, std::string> p1;
  Promise<void, std::string> p2 =
      p1.Then(FROM_HERE, BindOnce(
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

  p1.GetResolver()->Resolve();
  run_loop_.Run();

  std::vector<std::string> expected_log = {"Then #1",        "Then #2 (reject)",
                                           "Caught Whoops!", "Then #5",
                                           "Then #6",        "Finally"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, ThenChainsWithFinally) {
  std::vector<std::string> log;
  Promise<void> p;
  p.Finally(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Finally"); },
               &log));

  p.Then(FROM_HERE,
         BindOnce(
             [](std::vector<std::string>* log) { log->push_back("Then A1"); },
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

  p.Then(FROM_HERE,
         BindOnce(
             [](std::vector<std::string>* log) { log->push_back("Then B1"); },
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

  p.GetResolver()->Resolve();
  run_loop_.Run();

  std::vector<std::string> expected_log = {"Then A1", "Then B1", "Then A2",
                                           "Then B2", "Finally", "Then A3",
                                           "Then B3"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, Cancel) {
  std::vector<std::string> log;
  Promise<void, std::string> p1;
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

  p1.GetResolver()->Resolve();
  run_loop_.RunUntilIdle();

  EXPECT_TRUE(log.empty());
}

TEST_F(PromiseTest, RaceReject) {
  Promise<float, std::string> p1;
  Promise<int, std::string> p2;
  Promise<bool, std::string> p3;

  promise::Race(p1, p2, p3)
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

  p3.GetResolver()->Reject("Whoops!");
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolve) {
  PromiseResolver<int> promise_resolver;
  Promise<float> p1;
  Promise<int> p2;
  Promise<bool> p3;

  promise::Race(p1, p2, p3)
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Variant<float, int, bool> result) {
                  EXPECT_EQ(1337, Get<int>(&result));
                  run_loop->Quit();
                },
                &run_loop_));

  p2.GetResolver()->Resolve(1337);
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolveIntVoidVoidInt) {
  Promise<int> p1;
  Promise<void> p2;
  Promise<void> p3;
  Promise<int> p4;

  promise::Race(p1, p2, p3, p4)
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Variant<int, Void> result) {
                  EXPECT_TRUE(GetIf<Void>(&result));
                  run_loop->Quit();
                },
                &run_loop_));

  p3.GetResolver()->Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceRejectMultipleTypes) {
  Promise<int> p1;
  Promise<void, bool> p2;
  Promise<void, int> p3;
  Promise<int, std::string> p4;

  promise::Race(p1, p2, p3, p4)
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

  p4.GetResolver()->Reject("Oh dear!");
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolveMoveOnlyType) {
  Promise<std::unique_ptr<float>> p1;
  Promise<std::unique_ptr<int>> p2;
  Promise<std::unique_ptr<bool>> p3;

  promise::Race(p1, p2, p3)
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop,
                   Variant<std::unique_ptr<float>, std::unique_ptr<int>,
                           std::unique_ptr<bool>> result) {
                  EXPECT_EQ(1337, *Get<std::unique_ptr<int>>(&result));
                  run_loop->Quit();
                },
                &run_loop_));

  p2.GetResolver()->Resolve(std::make_unique<int>(1337));
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceResolveIntContainer) {
  std::vector<Promise<int>> promises{Promise<int>(), Promise<int>(),
                                     Promise<int>(), Promise<int>()};

  promise::Race(promises).Then(FROM_HERE,
                               BindOnce(
                                   [](base::RunLoop* run_loop, int result) {
                                     EXPECT_EQ(54321, result);
                                     run_loop->Quit();
                                   },
                                   &run_loop_));

  promises[3].GetResolver()->Resolve(54321);
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceRejectVoidContainer) {
  std::vector<Promise<void>> promises{Promise<void>(), Promise<void>(),
                                      Promise<void>(), Promise<void>()};

  promise::Race(promises).Catch(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  promises[3].GetResolver()->Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceVariantContainer) {
  std::vector<Variant<Promise<int>, Promise<bool>, Promise<std::string>>>
      promises;

  Promise<std::string> p;
  promises.push_back(Promise<int>());
  promises.push_back(Promise<bool>());
  promises.push_back(p);
  promises.push_back(Promise<bool>());

  promise::Race(promises).Then(
      FROM_HERE,
      BindOnce(
          [](base::RunLoop* run_loop, Variant<int, bool, std::string> result) {
            EXPECT_EQ("Hello", (Get<std::string>(&result)));
            run_loop->Quit();
          },
          &run_loop_));

  p.GetResolver()->Resolve("Hello");
  run_loop_.Run();
}

TEST_F(PromiseTest, RaceVariantContainerReject) {
  std::vector<Variant<Promise<void, std::string>, Promise<void, int>,
                      Promise<void, bool>>>
      promises;

  Promise<void, int> p;
  promises.push_back(Promise<void, std::string>());
  promises.push_back(Promise<void, std::string>());
  promises.push_back(p);
  promises.push_back(Promise<void, bool>());

  promise::Race(promises).Catch(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop,
                        const Variant<std::string, int, bool>& err) {
                       EXPECT_EQ(-1, Get<int>(&err));
                       run_loop->Quit();
                     },
                     &run_loop_));

  p.GetResolver()->Reject(-1);
  run_loop_.Run();
}

TEST_F(PromiseTest, All) {
  Promise<float> p1;
  Promise<int> p2;
  Promise<bool> p3;

  promise::All(p1, p2, p3)
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop,
                              std::tuple<float, int, bool> result) {
                             EXPECT_EQ(1.234f, std::get<0>(result));
                             EXPECT_EQ(1234, std::get<1>(result));
                             EXPECT_TRUE(std::get<2>(result));
                             run_loop->Quit();
                           },
                           &run_loop_));

  p1.GetResolver()->Resolve(1.234f);
  p2.GetResolver()->Resolve(1234);
  p3.GetResolver()->Resolve(true);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllUnpackTupleTuple) {
  Promise<float> p1;
  Promise<int> p2;
  Promise<bool> p3;

  promise::All(p1, p2, p3)
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, float a, int b, bool c) {
                             EXPECT_EQ(1.234f, a);
                             EXPECT_EQ(1234, b);
                             EXPECT_TRUE(c);
                             run_loop->Quit();
                           },
                           &run_loop_));

  p1.GetResolver()->Resolve(1.234f);
  p2.GetResolver()->Resolve(1234);
  p3.GetResolver()->Resolve(true);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllRejectString) {
  Promise<float, std::string> p1;
  Promise<int, std::string> p2;
  Promise<bool, std::string> p3;

  promise::All(p1, p2, p3)
      .Then(FROM_HERE, BindOnce([](std::tuple<float, int, bool> result) {
              FAIL() << "We shouldn't get here, the promise was rejected!";
            }),
            BindOnce(
                [](base::RunLoop* run_loop, const std::string& err) {
                  EXPECT_EQ("Whoops!", err);
                  run_loop->Quit();
                },
                &run_loop_));

  p1.GetResolver()->Reject("Whoops!");
  run_loop_.Run();
}

TEST_F(PromiseTest, AllRejectVoid) {
  Promise<float> p1;
  Promise<int> p2;
  Promise<bool> p3;

  promise::All(p1, p2, p3)
      .Then(FROM_HERE, BindOnce([](std::tuple<float, int, bool> result) {
              FAIL() << "We shouldn't get here, the promise was rejected!";
            }),
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop_));

  p1.GetResolver()->Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntVoid) {
  Promise<int> p1;
  Promise<void> p2;

  promise::All(p1, p2).Then(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop, std::tuple<int, Void> result) {
                       EXPECT_EQ(1234, std::get<0>(result));
                       run_loop->Quit();
                     },
                     &run_loop_));

  p1.GetResolver()->Resolve(1234);
  p2.GetResolver()->Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllMoveOnlyType) {
  Promise<std::unique_ptr<float>> p1;
  Promise<std::unique_ptr<int>> p2;
  Promise<std::unique_ptr<bool>> p3;

  promise::All(p1, p2, p3)
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

  p1.GetResolver()->Resolve(std::make_unique<float>(1.234f));
  p2.GetResolver()->Resolve(std::make_unique<int>(1234));
  p3.GetResolver()->Resolve(std::make_unique<bool>(true));
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntWithVoidThen) {
  Promise<int> p1;
  Promise<int> p2;
  Promise<int> p3;

  // You can choose to ignore the result.
  promise::All(p1, p2, p3)
      .Then(FROM_HERE,
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop_));

  p1.GetResolver()->Resolve(1);
  p2.GetResolver()->Resolve(2);
  p3.GetResolver()->Resolve(3);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidWithVoidThen) {
  Promise<void> p1;
  Promise<void> p2;
  Promise<void> p3;

  promise::All(p1, p2, p3)
      .Then(FROM_HERE,
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop_));

  p1.GetResolver()->Resolve();
  p2.GetResolver()->Resolve();
  p3.GetResolver()->Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntContainer) {
  std::vector<Promise<int>> promises{Promise<int>(), Promise<int>(),
                                     Promise<int>(), Promise<int>()};

  promise::All(promises).Then(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop, std::vector<int> result) {
                       EXPECT_THAT(result, ElementsAre(10, 20, 30, 40));
                       run_loop->Quit();
                     },
                     &run_loop_));

  promises[0].GetResolver()->Resolve(10);
  promises[1].GetResolver()->Resolve(20);
  promises[2].GetResolver()->Resolve(30);
  promises[3].GetResolver()->Resolve(40);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllIntStringContainerReject) {
  std::vector<Promise<int, std::string>> promises{
      Promise<int, std::string>(), Promise<int, std::string>(),
      Promise<int, std::string>(), Promise<int, std::string>()};

  promise::All(promises).Catch(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop, const std::string& err) {
                       EXPECT_EQ("Oh dear", err);
                       run_loop->Quit();
                     },
                     &run_loop_));

  promises[3].GetResolver()->Reject("Oh dear");
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidContainer) {
  std::vector<Promise<void>> promises{Promise<void>(), Promise<void>(),
                                      Promise<void>(), Promise<void>()};

  promise::All(promises).Then(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop, std::vector<Void> result) {
                       EXPECT_EQ(4u, result.size());
                       run_loop->Quit();
                     },
                     &run_loop_));

  promises[0].GetResolver()->Resolve();
  promises[1].GetResolver()->Resolve();
  promises[2].GetResolver()->Resolve();
  promises[3].GetResolver()->Resolve();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidIntContainerReject) {
  std::vector<Promise<void, int>> promises{
      Promise<void, int>(), Promise<void, int>(), Promise<void, int>(),
      Promise<void, int>()};

  promise::All(promises).Catch(FROM_HERE,
                               BindOnce(
                                   [](base::RunLoop* run_loop, int err) {
                                     EXPECT_EQ(-1, err);
                                     run_loop->Quit();
                                   },
                                   &run_loop_));

  promises[2].GetResolver()->Reject(-1);
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVoidContainerReject) {
  std::vector<Promise<void>> promises{Promise<void>(), Promise<void>(),
                                      Promise<void>(), Promise<void>()};

  promise::All(promises).Catch(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));

  promises[2].GetResolver()->Reject();
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVariantContainer) {
  Promise<int> p1;
  Promise<void> p2;
  Promise<std::string> p3;

  std::vector<Variant<Promise<int>, Promise<void>, Promise<std::string>>>
      promises;
  promises.push_back(p1);
  promises.push_back(p2);
  promises.push_back(p3);

  promise::All(promises).Then(
      FROM_HERE, BindOnce(
                     [](base::RunLoop* run_loop,
                        std::vector<Variant<int, Void, std::string>> result) {
                       EXPECT_EQ(10, (Get<int>(&result[0])));
                       EXPECT_EQ("three", (Get<std::string>(&result[2])));
                       run_loop->Quit();
                     },
                     &run_loop_));

  p1.GetResolver()->Resolve(10);
  p2.GetResolver()->Resolve();
  p3.GetResolver()->Resolve(std::string("three"));
  run_loop_.Run();
}

TEST_F(PromiseTest, AllVariantContainerReject) {
  Promise<int, int> p1;
  Promise<void, int> p2;
  Promise<std::string, int> p3;

  std::vector<
      Variant<Promise<int, int>, Promise<void, int>, Promise<std::string, int>>>
      promises;
  promises.push_back(p1);
  promises.push_back(p2);
  promises.push_back(p3);

  promise::All(promises).Catch(FROM_HERE,
                               BindOnce(
                                   [](base::RunLoop* run_loop, int err) {
                                     EXPECT_EQ(-1, err);
                                     run_loop->Quit();
                                   },
                                   &run_loop_));

  p2.GetResolver()->Reject(-1);
  run_loop_.Run();
}

}  // namespace base
