// Copyright 2017 The Chromium Authors. All rights reserved.
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

TEST_F(PromiseTest, ResolveThen) {
  Promise<int> p =
      Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(123); }));

  p.Then(BindOnce([](int result) { EXPECT_EQ(123, result); }));
}

TEST_F(PromiseTest, RejectThen) {
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int> pr) { pr.reject(Value("Oh no!")); }));

  p.Then(BindOnce([](int result) {
           FAIL() << "We shouldn't get here, the promise was rejected!";
         }),
         BindOnce([](const RejectValue& err) {
           EXPECT_EQ("Oh no!", err->GetString());
         }));
}

TEST_F(PromiseTest, ResolveOutsidePriomiseBeforeThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));

  promise_resolver.resolve(123);
  bool called = false;
  p.Then(BindOnce(
      [](bool* called, int result) {
        *called = true;
        EXPECT_EQ(123, result);
      },
      &called));
  EXPECT_TRUE(called);
}

TEST_F(PromiseTest, RejectOutsidePriomiseBeforeThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));

  promise_resolver.reject(Value("Oh no!"));
  bool called = false;
  p.Then(BindOnce([](int result) {
           FAIL() << "We shouldn't get here, the promise was rejected!";
         }),
         BindOnce(
             [](bool* called, const RejectValue& err) {
               *called = true;
               EXPECT_EQ("Oh no!", err->GetString());
             },
             &called));
  EXPECT_TRUE(called);
}

TEST_F(PromiseTest, ResolveOutsidePriomiseAfterThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));

  bool called = false;
  p.Then(BindOnce(
      [](bool* called, int result) {
        *called = true;
        EXPECT_EQ(123, result);
      },
      &called));

  EXPECT_FALSE(called);
  promise_resolver.resolve(123);
  EXPECT_TRUE(called);
}

TEST_F(PromiseTest, RejectOutsidePriomiseAfterThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));

  bool called = false;
  p.Then(BindOnce([](int result) {
           FAIL() << "We shouldn't get here, the promise was rejected!";
         }),
         BindOnce(
             [](bool* called, const RejectValue& err) {
               *called = true;
               EXPECT_EQ("Oh no!", err->GetString());
             },
             &called));

  EXPECT_FALSE(called);
  promise_resolver.reject(Value("Oh no!"));
  EXPECT_TRUE(called);
}

TEST_F(PromiseTest, ThenChain) {
  Promise<std::vector<size_t>>(
      BindOnce(
          [](PromiseResolver<std::vector<size_t>> pr) { pr.resolve({0}); }))
      .Then(BindOnce([](std::vector<size_t> result) {
        result.push_back(result.size());
        return result;
      }))
      .Then(BindOnce([](std::vector<size_t> result) {
        result.push_back(result.size());
        return result;
      }))
      .Then(BindOnce([](std::vector<size_t> result) {
        result.push_back(result.size());
        return result;
      }))
      .Then(BindOnce([](std::vector<size_t> result) {
        EXPECT_THAT(result, ElementsAre(0u, 1u, 2u, 3u));
      }));
}

TEST_F(PromiseTest, RejectionInThenChain) {
  Promise<std::vector<size_t>>(
      BindOnce(
          [](PromiseResolver<std::vector<size_t>> pr) { pr.resolve({0}); }))
      .Then(BindOnce([](std::vector<size_t> result) {
        result.push_back(result.size());
        return result;
      }))
      .Then(BindOnce([](std::vector<size_t> result) {
        result.push_back(result.size());
        return result;
      }))
      .Then(BindOnce(
          [](std::vector<size_t> result) -> PromiseResult<std::vector<size_t>> {
            return RejectValue(Value("Oh no!"));
          }))
      .Then(BindOnce([](std::vector<size_t> result) {
              FAIL() << "We shouldn't get here, the promise was rejected!";
            }),
            BindOnce([](const RejectValue& err) {
              EXPECT_EQ("Oh no!", err->GetString());
            }));
}

TEST_F(PromiseTest, ThenChainVariousReturnTypes) {
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
      .Then(BindOnce([]() { return 5; }))
      .Then(BindOnce([](int result) {
        EXPECT_EQ(5, result);
        return std::string("Hello");
      }))
      .Then(BindOnce([](std::string result) {
        EXPECT_EQ("Hello", result);
        return true;
      }))
      .Then(BindOnce([](bool result) { EXPECT_TRUE(result); }));
}

TEST_F(PromiseTest, CurriedVoidPromise) {
  PromiseResolver<void> promise_resolver;

  bool called = false;
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
      .Then(BindOnce(
          [](PromiseResolver<void>* promise_resolver) {
            return Promise<void>(BindOnce(
                [](PromiseResolver<void>* promise_resolver,
                   PromiseResolver<void> pr) { *promise_resolver = pr; },
                promise_resolver));
          },
          &promise_resolver))
      .Then(BindOnce([](bool* called) { *called = true; }, &called));

  EXPECT_FALSE(called);
  promise_resolver.resolve();
  EXPECT_TRUE(called);
}

TEST_F(PromiseTest, CurriedIntPromise) {
  PromiseResolver<int> promise_resolver;

  bool called = false;
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(1000); }))
      .Then(BindOnce(
          [](PromiseResolver<int>* promise_resolver, int result) {
            EXPECT_EQ(1000, result);
            return Promise<int>(BindOnce(
                [](PromiseResolver<int>* promise_resolver,
                   PromiseResolver<int> pr) { *promise_resolver = pr; },
                promise_resolver));
          },
          &promise_resolver))
      .Then(BindOnce(
          [](bool* called, int result) {
            *called = true;
            EXPECT_EQ(123, result);
          },
          &called));

  EXPECT_FALSE(called);
  promise_resolver.resolve(123);
  EXPECT_TRUE(called);
}

TEST_F(PromiseTest, ResolveToDisambiguateThenReturnValue) {
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
      .Then(BindOnce([]() -> PromiseResult<Value> { return Value("Hello."); }))
      .Then(BindOnce(
          [](Value result) { EXPECT_EQ("Hello.", result.GetString()); }));
}

TEST_F(PromiseTest, RejectToDisambiguateThenReturnValue) {
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
      .Then(BindOnce([]() -> PromiseResult<Value> {
        return RejectValue(Value("Oh no!"));
      }))
      .Then(BindOnce([](Value result) {
              FAIL() << "We shouldn't get here, the promise was rejected!";
            }),
            BindOnce([](const RejectValue& err) {
              EXPECT_EQ("Oh no!", err->GetString());
            }));
}

void RecordOrder(std::vector<int>* run_order, int order) {
  run_order->push_back(order);
}

TEST_F(PromiseTest, BranchedThenChainExecutionOrder) {
  std::vector<int> run_order;
  PromiseResolver<void> promise_resolver;
  Promise<void> promise_a = Promise<void>(
      BindOnce([](PromiseResolver<void>* promise_resolver,
                  PromiseResolver<void> pr) { *promise_resolver = pr; },
               &promise_resolver));

  Promise<void> promise_b =
      promise_a.Then(base::BindOnce(&RecordOrder, &run_order, 0))
          .Then(base::BindOnce(&RecordOrder, &run_order, 1));

  Promise<void> promise_c =
      promise_b.Then(base::BindOnce(&RecordOrder, &run_order, 2))
          .Then(base::BindOnce(&RecordOrder, &run_order, 3));

  Promise<void> promise_d =
      promise_b.Then(base::BindOnce(&RecordOrder, &run_order, 4))
          .Then(base::BindOnce(&RecordOrder, &run_order, 5));

  promise_resolver.resolve();
  EXPECT_THAT(run_order, ElementsAre(0, 1, 2, 4, 3, 5));
}

TEST_F(PromiseTest, Catch) {
  bool caught = false;
  Promise<int>(
      BindOnce([](PromiseResolver<int> pr) { pr.reject(Value("Whoops!")); }))
      .Then(BindOnce([](int result) { return result; }))
      .Then(BindOnce([](int result) { return result; }))
      .Then(BindOnce([](int result) { return result; }))
      .Catch(BindOnce(
          [](bool* caught, const RejectValue& err) {
            EXPECT_EQ("Whoops!", err->GetString());
            *caught = true;
          },
          &caught));

  EXPECT_TRUE(caught);
}

TEST_F(PromiseTest, CatchRejectInThenChain) {
  bool caught = false;
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(123); }))
      .Then(BindOnce([](int result) -> PromiseResult<int> {
        return RejectValue(Value("Whoops!"));
      }))
      .Then(BindOnce([](int result) { return result; }))
      .Then(BindOnce([](int result) { return result; }))
      .Catch(BindOnce(
          [](bool* caught, const RejectValue& err) {
            EXPECT_EQ("Whoops!", err->GetString());
            *caught = true;
          },
          &caught));

  EXPECT_TRUE(caught);
}

TEST_F(PromiseTest, CatchThen) {
  int err_value = 0;
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.reject(Value(123)); }))
      .Catch(BindOnce([](const RejectValue& err) { return err->GetInt(); }))
      .Then(BindOnce([](int* err_value, int result) { *err_value = result; },
                     &err_value));

  EXPECT_EQ(123, err_value);
}

TEST_F(PromiseTest, SettledTaskFinally) {
  int result = 0;
  bool finally_run = false;
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(123); }))
      .Then(BindOnce([](int* result, int value) { *result = value; }, &result))
      .Finally(BindOnce(
          [](int* result, bool* finally_run) {
            EXPECT_EQ(123, *result);
            *finally_run = true;
          },
          &result, &finally_run));
  EXPECT_TRUE(finally_run);
}

TEST_F(PromiseTest, ResolveFinally) {
  int result = 0;
  PromiseResolver<int> promise_resolver;
  Promise<int> p =
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver);

  p.Then(BindOnce([](int* result, int value) { *result = value; }, &result));
  bool finally_run = false;
  p.Finally(BindOnce(
      [](int* result, bool* finally_run) {
        EXPECT_EQ(123, *result);
        *finally_run = true;
      },
      &result, &finally_run));
  promise_resolver.resolve(123);
  EXPECT_TRUE(finally_run);
}

TEST_F(PromiseTest, RejectFinally) {
  int result = 0;
  PromiseResolver<int> promise_resolver;
  Promise<int> p =
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver);
  p.Then(BindOnce([](int* result, int value) { *result = value; }, &result),
         BindOnce([](int* result, const RejectValue& err) { *result = -1; },
                  &result));
  bool finally_run = false;
  p.Finally(BindOnce(
      [](int* result, bool* finally_run) {
        EXPECT_EQ(-1, *result);
        *finally_run = true;
      },
      &result, &finally_run));
  promise_resolver.reject(Value("Oh no!"));
  EXPECT_TRUE(finally_run);
}

TEST_F(PromiseTest, CatchThenFinally) {
  std::vector<std::string> log;
  PromiseResolver<void> promise_resolver;
  Promise<void> p =
      Promise<void>(BindOnce(
                        [](std::vector<std::string>* log,
                           PromiseResolver<void>* promise_resolver,
                           PromiseResolver<void> pr) {
                          log->push_back("Initial resolve");
                          *promise_resolver = pr;
                        },
                        &log, &promise_resolver))
          .Then(BindOnce(
              [](std::vector<std::string>* log) { log->push_back("Then #1"); },
              &log))
          .Then(BindOnce(
              [](std::vector<std::string>* log) -> PromiseResult<void> {
                log->push_back("Then #2 (reject)");
                return RejectValue(Value("Whoops!"));
              },
              &log))
          .Then(BindOnce(
              [](std::vector<std::string>* log) { log->push_back("Then #3"); },
              &log))
          .Then(BindOnce(
              [](std::vector<std::string>* log) { log->push_back("Then #4"); },
              &log))
          .Catch(BindOnce(
              [](std::vector<std::string>* log, const RejectValue& err) {
                log->push_back("Caught " + err->GetString());
              },
              &log));

  p.Finally(BindOnce(
      [](std::vector<std::string>* log) { log->push_back("Finally"); }, &log));
  p.Then(BindOnce(
      [](std::vector<std::string>* log) { log->push_back("Then #5"); }, &log));
  p.Then(BindOnce(
      [](std::vector<std::string>* log) { log->push_back("Then #6"); }, &log));

  promise_resolver.resolve();
  std::vector<std::string> expected_log = {
      "Initial resolve", "Then #1", "Then #2 (reject)", "Caught Whoops!",
      "Then #5",         "Then #6", "Finally"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, ThenChainsWithFinally) {
  std::vector<std::string> log;
  PromiseResolver<void> promise_resolver;
  Promise<void> p = Promise<void>(BindOnce(
      [](std::vector<std::string>* log, PromiseResolver<void>* promise_resolver,
         PromiseResolver<void> pr) {
        log->push_back("Initial resolve");
        *promise_resolver = pr;
      },
      &log, &promise_resolver));

  p.Finally(BindOnce(
      [](std::vector<std::string>* log) { log->push_back("Finally"); }, &log));

  p.Then(BindOnce(
             [](std::vector<std::string>* log) { log->push_back("Then A1"); },
             &log))
      .Then(BindOnce(
          [](std::vector<std::string>* log) { log->push_back("Then A2"); },
          &log))
      .Then(BindOnce(
          [](std::vector<std::string>* log) { log->push_back("Then A3"); },
          &log));

  p.Then(BindOnce(
             [](std::vector<std::string>* log) { log->push_back("Then B1"); },
             &log))
      .Then(BindOnce(
          [](std::vector<std::string>* log) { log->push_back("Then B2"); },
          &log))
      .Then(BindOnce(
          [](std::vector<std::string>* log) { log->push_back("Then B3"); },
          &log));

  promise_resolver.resolve();
  std::vector<std::string> expected_log = {
      "Initial resolve", "Then A1", "Then B1", "Then A2",
      "Then B2",         "Finally", "Then A3", "Then B3"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, PromiseReject) {
  promise::Reject<int>(Value("Oh no!"))
      .Then(BindOnce([](int result) {
              FAIL() << "We shouldn't get here, the promise was rejected!";
            }),
            BindOnce([](const RejectValue& err) {
              EXPECT_EQ("Oh no!", err->GetString());
            }));
}

TEST_F(PromiseTest, PromiseResolve) {
  promise::Resolve<int>(1000).Then(
      BindOnce([](int result) { EXPECT_EQ(1000, result); }),
      BindOnce([](const RejectValue& err) {
        FAIL() << "We shouldn't get here, the promise was resolved!";
      }));
}

TEST_F(PromiseTest, RaceReject) {
  PromiseResolver<int> promise_resolver;
  Promise<float> p1 =
      Promise<float>(BindOnce([](PromiseResolver<float> pr) {}));
  Promise<int> p2 = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));
  Promise<bool> p3 = Promise<bool>(BindOnce([](PromiseResolver<bool> pr) {}));

  promise::Race(p1, p2, p3)
      .Then(BindOnce([](Variant<float, int, bool> result) {
              FAIL() << "We shouldn't get here, a raced promise was rejected!";
            }),
            BindOnce([](const RejectValue& err) {
              EXPECT_EQ("Whoops!", err->GetString());
            }));

  promise_resolver.reject(Value("Whoops!"));
}

TEST_F(PromiseTest, RaceResolve) {
  PromiseResolver<int> promise_resolver;
  Promise<float> p1 =
      Promise<float>(BindOnce([](PromiseResolver<float> pr) {}));
  Promise<int> p2 = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));
  Promise<bool> p3 = Promise<bool>(BindOnce([](PromiseResolver<bool> pr) {}));

  bool race_fired = false;
  promise::Race(p1, p2, p3)
      .Then(BindOnce(
          [](bool* race_fired, Variant<float, int, bool> result) {
            EXPECT_EQ(1337, Get<int>(&result));
            *race_fired = true;
          },
          &race_fired));

  EXPECT_FALSE(race_fired);
  promise_resolver.resolve(1337);
  EXPECT_TRUE(race_fired);
}

TEST_F(PromiseTest, RaceResolveIntVoidVoidInt) {
  PromiseResolver<void> promise_resolver;
  Promise<int> p1 = Promise<int>(BindOnce([](PromiseResolver<int> pr) {}));
  Promise<void> p2 = Promise<void>(
      BindOnce([](PromiseResolver<void>* promise_resolver,
                  PromiseResolver<void> pr) { *promise_resolver = pr; },
               &promise_resolver));
  Promise<void> p3 = Promise<void>(BindOnce([](PromiseResolver<void> pr) {}));
  Promise<int> p4 = Promise<int>(BindOnce([](PromiseResolver<int> pr) {}));

  bool race_fired = false;
  promise::Race(p1, p2, p3, p4)
      .Then(BindOnce(
          [](bool* race_fired, Variant<Void, int> result) {
            EXPECT_TRUE(GetIf<Void>(&result));
            *race_fired = true;
          },
          &race_fired));

  EXPECT_FALSE(race_fired);
  promise_resolver.resolve();
  EXPECT_TRUE(race_fired);
}

TEST_F(PromiseTest, RaceRejectIntVoidVoidInt) {
  PromiseResolver<void> promise_resolver;
  Promise<int> p1 = Promise<int>(BindOnce([](PromiseResolver<int> pr) {}));
  Promise<void> p2 = Promise<void>(
      BindOnce([](PromiseResolver<void>* promise_resolver,
                  PromiseResolver<void> pr) { *promise_resolver = pr; },
               &promise_resolver));
  Promise<void> p3 = Promise<void>(BindOnce([](PromiseResolver<void> pr) {}));
  Promise<int> p4 = Promise<int>(BindOnce([](PromiseResolver<int> pr) {}));

  bool race_fired = false;
  promise::Race(p1, p2, p3, p4)
      .Then(BindOnce([](Variant<Void, int> result) {
              FAIL() << "One of the promises was rejected.";
            }),
            BindOnce(
                [](bool* race_fired, const RejectValue& err) {
                  EXPECT_EQ("Oh dear!", err->GetString());
                  *race_fired = true;
                },
                &race_fired));

  EXPECT_FALSE(race_fired);
  promise_resolver.reject(Value("Oh dear!"));
  EXPECT_TRUE(race_fired);
}

TEST_F(PromiseTest, RaceResolveMoveOnlyType) {
  PromiseResolver<std::unique_ptr<int>> promise_resolver;
  Promise<std::unique_ptr<float>> p1 = Promise<std::unique_ptr<float>>(
      BindOnce([](PromiseResolver<std::unique_ptr<float>> pr) {}));
  Promise<std::unique_ptr<int>> p2 = Promise<std::unique_ptr<int>>(BindOnce(
      [](PromiseResolver<std::unique_ptr<int>>* promise_resolver,
         PromiseResolver<std::unique_ptr<int>> pr) { *promise_resolver = pr; },
      &promise_resolver));
  Promise<std::unique_ptr<bool>> p3 = Promise<std::unique_ptr<bool>>(
      BindOnce([](PromiseResolver<std::unique_ptr<bool>> pr) {}));

  bool race_fired = false;
  promise::Race(p1, p2, p3)
      .Then(BindOnce(
          [](bool* race_fired,
             Variant<std::unique_ptr<float>, std::unique_ptr<int>,
                     std::unique_ptr<bool>> result) {
            EXPECT_EQ(1337, *Get<std::unique_ptr<int>>(&result));
            *race_fired = true;
          },
          &race_fired));

  EXPECT_FALSE(race_fired);
  promise_resolver.resolve(std::make_unique<int>(1337));
  EXPECT_TRUE(race_fired);
}

TEST_F(PromiseTest, All) {
  PromiseResolver<float> promise_resolver1;
  PromiseResolver<int> promise_resolver2;
  PromiseResolver<bool> promise_resolver3;
  Promise<float> p1 = Promise<float>(
      BindOnce([](PromiseResolver<float>* promise_resolver1,
                  PromiseResolver<float> pr) { *promise_resolver1 = pr; },
               &promise_resolver1));
  Promise<int> p2 = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver2,
                  PromiseResolver<int> pr) { *promise_resolver2 = pr; },
               &promise_resolver2));
  Promise<bool> p3 = Promise<bool>(
      BindOnce([](PromiseResolver<bool>* promise_resolver3,
                  PromiseResolver<bool> pr) { *promise_resolver3 = pr; },
               &promise_resolver3));

  bool all_fired = false;
  promise::All(p1, p2, p3)
      .Then(BindOnce(
          [](bool* all_fired,
             std::tuple<Variant<float, RejectValue>, Variant<int, RejectValue>,
                        Variant<bool, RejectValue>> result) {
            EXPECT_EQ(1.234f, Get<float>(&std::get<0>(result)));
            EXPECT_EQ(1234, Get<int>(&std::get<1>(result)));
            EXPECT_TRUE(Get<bool>(&std::get<2>(result)));
            *all_fired = true;
          },
          &all_fired));

  promise_resolver1.resolve(1.234f);
  promise_resolver2.resolve(1234);
  EXPECT_FALSE(all_fired);

  promise_resolver3.resolve(true);
  EXPECT_TRUE(all_fired);
}

TEST_F(PromiseTest, AllIntVoid) {
  PromiseResolver<int> promise_resolver1;
  PromiseResolver<void> promise_resolver2;
  Promise<int> p1 = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver1,
                  PromiseResolver<int> pr) { *promise_resolver1 = pr; },
               &promise_resolver1));
  Promise<void> p2 = Promise<void>(
      BindOnce([](PromiseResolver<void>* promise_resolver2,
                  PromiseResolver<void> pr) { *promise_resolver2 = pr; },
               &promise_resolver2));

  bool all_fired = false;
  promise::All(p1, p2).Then(BindOnce(
      [](bool* all_fired, std::tuple<Variant<int, RejectValue>,
                                     Variant<Void, RejectValue>> result) {
        EXPECT_EQ(1234, Get<int>(&std::get<0>(result)));
        EXPECT_TRUE(GetIf<Void>(&std::get<1>(result)));
        *all_fired = true;
      },
      &all_fired));

  promise_resolver1.resolve(1234);
  EXPECT_FALSE(all_fired);

  promise_resolver2.resolve();
  EXPECT_TRUE(all_fired);
}

TEST_F(PromiseTest, AllMoveOnlyType) {
  PromiseResolver<std::unique_ptr<float>> promise_resolver1;
  PromiseResolver<std::unique_ptr<int>> promise_resolver2;
  PromiseResolver<std::unique_ptr<bool>> promise_resolver3;
  Promise<std::unique_ptr<float>> p1 = Promise<std::unique_ptr<float>>(BindOnce(
      [](PromiseResolver<std::unique_ptr<float>>* promise_resolver1,
         PromiseResolver<std::unique_ptr<float>> pr) {
        *promise_resolver1 = pr;
      },
      &promise_resolver1));
  Promise<std::unique_ptr<int>> p2 = Promise<std::unique_ptr<int>>(BindOnce(
      [](PromiseResolver<std::unique_ptr<int>>* promise_resolver2,
         PromiseResolver<std::unique_ptr<int>> pr) { *promise_resolver2 = pr; },
      &promise_resolver2));
  Promise<std::unique_ptr<bool>> p3 = Promise<std::unique_ptr<bool>>(BindOnce(
      [](PromiseResolver<std::unique_ptr<bool>>* promise_resolver3,
         PromiseResolver<std::unique_ptr<bool>> pr) {
        *promise_resolver3 = pr;
      },
      &promise_resolver3));

  promise::All(p1, p2, p3)
      .Then(BindOnce(
          [](std::tuple<Variant<std::unique_ptr<float>, RejectValue>,
                        Variant<std::unique_ptr<int>, RejectValue>,
                        Variant<std::unique_ptr<bool>, RejectValue>> result) {
            EXPECT_EQ(1.234f,
                      *Get<std::unique_ptr<float>>(&std::get<0>(result)));
            EXPECT_EQ(1234, *Get<std::unique_ptr<int>>(&std::get<1>(result)));
            EXPECT_TRUE(*Get<std::unique_ptr<bool>>(&std::get<2>(result)));
          }));

  promise_resolver1.resolve(std::make_unique<float>(1.234f));
  promise_resolver2.resolve(std::make_unique<int>(1234));
  promise_resolver3.resolve(std::make_unique<bool>(true));
}

TEST_F(PromiseTest, PostTaskResolveThen) {
  Promise<int> p =
      Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(123); }));

  p.Then(FROM_HERE, BindOnce(
                        [](base::RunLoop* run_loop, int result) {
                          EXPECT_EQ(123, result);
                          run_loop->Quit();
                        },
                        &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskRejectThen) {
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int> pr) { pr.reject(Value("Oh no!")); }));

  p.Then(FROM_HERE,
         BindOnce(
             [](base::RunLoop* run_loop, int result) {
               FAIL() << "We shouldn't get here, the promise was rejected!";
               run_loop->Quit();
             },
             &run_loop_),
         BindOnce(
             [](base::RunLoop* run_loop, const RejectValue& err) {
               EXPECT_EQ("Oh no!", err->GetString());
               run_loop->Quit();
             },
             &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskResolveOutsidePriomiseBeforeThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));

  promise_resolver.resolve(123);
  p.Then(FROM_HERE, BindOnce(
                        [](base::RunLoop* run_loop, int result) {
                          EXPECT_EQ(123, result);
                          run_loop->Quit();
                        },
                        &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskRejectOutsidePriomiseBeforeThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));

  promise_resolver.reject(Value("Oh no!"));
  p.Then(FROM_HERE,
         BindOnce(
             [](base::RunLoop* run_loop, int result) {
               FAIL() << "We shouldn't get here, the promise was rejected!";
               run_loop->Quit();
             },
             &run_loop_),
         BindOnce(
             [](base::RunLoop* run_loop, const RejectValue& err) {
               EXPECT_EQ("Oh no!", err->GetString());
               run_loop->Quit();
             },
             &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskResolveOutsidePriomiseAfterThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));
  p.Then(FROM_HERE, BindOnce(
                        [](base::RunLoop* run_loop, int result) {
                          EXPECT_EQ(123, result);
                          run_loop->Quit();
                        },
                        &run_loop_));

  promise_resolver.resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskRejectOutsidePriomiseAfterThen) {
  PromiseResolver<int> promise_resolver;
  Promise<int> p = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));

  p.Then(FROM_HERE,
         BindOnce(
             [](base::RunLoop* run_loop, int result) {
               FAIL() << "We shouldn't get here, the promise was rejected!";
               run_loop->Quit();
             },
             &run_loop_),
         BindOnce(
             [](base::RunLoop* run_loop, const RejectValue& err) {
               EXPECT_EQ("Oh no!", err->GetString());
               run_loop->Quit();
             },
             &run_loop_));

  promise_resolver.reject(Value("Oh no!"));
  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskThenChain) {
  Promise<std::vector<size_t>>(
      BindOnce(
          [](PromiseResolver<std::vector<size_t>> pr) { pr.resolve({0}); }))
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

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskRejectionInThenChain) {
  Promise<std::vector<size_t>>(
      BindOnce(
          [](PromiseResolver<std::vector<size_t>> pr) { pr.resolve({0}); }))
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
              return RejectValue(Value("Oh no!"));
            }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, std::vector<size_t> result) {
                  FAIL() << "We shouldn't get here, the promise was rejected!";
                  run_loop->Quit();
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop, const RejectValue& err) {
                  EXPECT_EQ("Oh no!", err->GetString());
                  run_loop->Quit();
                },
                &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskThenChainVariousReturnTypes) {
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
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

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskCurriedVoidPromise) {
  PromiseResolver<void> promise_resolver;

  // We're not allowed to call Run multiple times so we need two run loops.
  base::RunLoop run_loop2;
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
      .Then(FROM_HERE,
            BindOnce(
                [](PromiseResolver<void>* promise_resolver) {
                  return Promise<void>(BindOnce(
                      [](PromiseResolver<void>* promise_resolver,
                         PromiseResolver<void> pr) { *promise_resolver = pr; },
                      promise_resolver));
                },
                &promise_resolver))
      .Then(FROM_HERE,
            BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     &run_loop2));
  run_loop_.RunUntilIdle();

  promise_resolver.resolve();
  run_loop2.Run();
}

TEST_F(PromiseTest, PostTaskCurriedIntPromise) {
  PromiseResolver<int> promise_resolver;

  // We're not allowed to call Run multiple times so we need two run loops.
  base::RunLoop run_loop2;
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(1000); }))
      .Then(FROM_HERE,
            BindOnce(
                [](PromiseResolver<int>* promise_resolver, int result) {
                  EXPECT_EQ(1000, result);
                  return Promise<int>(BindOnce(
                      [](PromiseResolver<int>* promise_resolver,
                         PromiseResolver<int> pr) { *promise_resolver = pr; },
                      promise_resolver));
                },
                &promise_resolver))
      .Then(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(123, result);
                             run_loop->Quit();
                           },
                           &run_loop2));
  run_loop_.RunUntilIdle();

  promise_resolver.resolve(123);
  run_loop2.Run();
}

TEST_F(PromiseTest, PostTaskResolveToDisambiguateThenReturnValue) {
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
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

TEST_F(PromiseTest, PostTaskRejectToDisambiguateThenReturnValue) {
  Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }))
      .Then(FROM_HERE, BindOnce([]() -> PromiseResult<Value> {
              return RejectValue(Value("Oh no!"));
            }))
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Value result) {
                  FAIL() << "We shouldn't get here, the promise was rejected!";
                  run_loop->Quit();
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop, const RejectValue& err) {
                  EXPECT_EQ("Oh no!", err->GetString());
                  run_loop->Quit();
                },
                &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskCatch) {
  Promise<int>(
      BindOnce([](PromiseResolver<int> pr) { pr.reject(Value("Whoops!")); }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Then(FROM_HERE, BindOnce([](int result) { return result; }))
      .Catch(FROM_HERE,
             BindOnce(
                 [](base::RunLoop* run_loop, const RejectValue& err) {
                   EXPECT_EQ("Whoops!", err->GetString());
                   run_loop->Quit();
                 },
                 &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskBranchedThenChainExecutionOrder) {
  std::vector<int> run_order;

  Promise<void> promise_a =
      Promise<void>(BindOnce([](PromiseResolver<void> pr) { pr.resolve(); }));

  Promise<void> promise_b =
      promise_a.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 0))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 1));

  Promise<void> promise_c =
      promise_b.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 2))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 3));

  Promise<void> promise_d =
      promise_b.Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 4))
          .Then(FROM_HERE, base::BindOnce(&RecordOrder, &run_order, 5));

  promise_d.Finally(
      FROM_HERE,
      BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); }, &run_loop_));
  run_loop_.Run();

  EXPECT_THAT(run_order, ElementsAre(0, 1, 2, 4, 3, 5));
}

TEST_F(PromiseTest, PostTaskCatchRejectInThenChain) {
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(123); }))
      .Then(BindOnce([](int result) -> PromiseResult<int> {
        return RejectValue(Value("Whoops!"));
      }))
      .Then(BindOnce([](int result) { return result; }))
      .Then(BindOnce([](int result) { return result; }))
      .Catch(BindOnce(
          [](base::RunLoop* run_loop, const RejectValue& err) {
            EXPECT_EQ("Whoops!", err->GetString());
            run_loop->Quit();
          },
          &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskCatchThen) {
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.reject(Value(123)); }))
      .Catch(BindOnce([](const RejectValue& err) { return err->GetInt(); }))
      .Then(BindOnce(
          [](base::RunLoop* run_loop, int result) {
            EXPECT_EQ(123, result);
            run_loop->Quit();
          },
          &run_loop_));

  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskSettledTaskFinally) {
  int result = 0;
  Promise<int>(BindOnce([](PromiseResolver<int> pr) { pr.resolve(123); }))
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

TEST_F(PromiseTest, PostTaskResolveFinally) {
  int result = 0;
  PromiseResolver<int> promise_resolver;
  Promise<int> p =
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver);
  p.Then(FROM_HERE,
         BindOnce([](int* result, int value) { *result = value; }, &result));
  p.Finally(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int* result) {
                             EXPECT_EQ(123, *result);
                             run_loop->Quit();
                           },
                           &run_loop_, &result));
  promise_resolver.resolve(123);
  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskRejectFinally) {
  int result = 0;
  PromiseResolver<int> promise_resolver;
  Promise<int> p =
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver);
  p.Then(FROM_HERE,
         BindOnce([](int* result, int value) { *result = value; }, &result),
         BindOnce([](int* result, const RejectValue& err) { *result = -1; },
                  &result));
  p.Finally(FROM_HERE, BindOnce(
                           [](base::RunLoop* run_loop, int* result) {
                             EXPECT_EQ(-1, *result);
                             run_loop->Quit();
                           },
                           &run_loop_, &result));
  promise_resolver.reject(Value("Oh no!"));
  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskCatchThenFinally) {
  std::vector<std::string> log;
  PromiseResolver<void> promise_resolver;
  Promise<void> p =
      Promise<void>(BindOnce(
                        [](std::vector<std::string>* log,
                           PromiseResolver<void>* promise_resolver,
                           PromiseResolver<void> pr) {
                          log->push_back("Initial resolve");
                          *promise_resolver = pr;
                        },
                        &log, &promise_resolver))
          .Then(FROM_HERE, BindOnce(
                               [](std::vector<std::string>* log) {
                                 log->push_back("Then #1");
                               },
                               &log))
          .Then(FROM_HERE,
                BindOnce(
                    [](std::vector<std::string>* log) -> PromiseResult<void> {
                      log->push_back("Then #2 (reject)");
                      return RejectValue(Value("Whoops!"));
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
                     [](std::vector<std::string>* log, const RejectValue& err) {
                       log->push_back("Caught " + err->GetString());
                     },
                     &log));

  p.Finally(FROM_HERE,
            BindOnce(
                [](std::vector<std::string>* log, base::RunLoop* run_loop) {
                  log->push_back("Finally");
                  run_loop->Quit();
                },
                &log, &run_loop_));
  p.Then(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Then #5"); },
               &log));
  p.Then(
      FROM_HERE,
      BindOnce([](std::vector<std::string>* log) { log->push_back("Then #6"); },
               &log));

  promise_resolver.resolve();
  run_loop_.Run();

  std::vector<std::string> expected_log = {
      "Initial resolve", "Then #1", "Then #2 (reject)", "Caught Whoops!",
      "Then #5",         "Then #6", "Finally"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, PostTaskThenChainsWithFinally) {
  std::vector<std::string> log;
  PromiseResolver<void> promise_resolver;
  Promise<void> p = Promise<void>(BindOnce(
      [](std::vector<std::string>* log, PromiseResolver<void>* promise_resolver,
         PromiseResolver<void> pr) {
        log->push_back("Initial resolve");
        *promise_resolver = pr;
      },
      &log, &promise_resolver));

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

  promise_resolver.resolve();
  run_loop_.Run();

  std::vector<std::string> expected_log = {
      "Initial resolve", "Then A1", "Then B1", "Then A2",
      "Then B2",         "Finally", "Then A3", "Then B3"};
  EXPECT_THAT(log, ElementsAreArray(expected_log));
}

TEST_F(PromiseTest, PostTaskRaceReject) {
  PromiseResolver<int> promise_resolver;
  Promise<float> p1 =
      Promise<float>(BindOnce([](PromiseResolver<float> pr) {}));
  Promise<int> p2 = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));
  Promise<bool> p3 = Promise<bool>(BindOnce([](PromiseResolver<bool> pr) {}));

  promise::Race(p1, p2, p3)
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Variant<float, int, bool> result) {
                  FAIL()
                      << "We shouldn't get here, a raced promise was rejected!";
                  run_loop->Quit();
                },
                &run_loop_),
            BindOnce(
                [](base::RunLoop* run_loop, const RejectValue& err) {
                  EXPECT_EQ("Whoops!", err->GetString());
                  run_loop->Quit();
                },
                &run_loop_));

  promise_resolver.reject(Value("Whoops!"));
  run_loop_.Run();
}

TEST_F(PromiseTest, PostTaskRaceResolve) {
  PromiseResolver<int> promise_resolver;
  Promise<float> p1 =
      Promise<float>(BindOnce([](PromiseResolver<float> pr) {}));
  Promise<int> p2 = Promise<int>(
      BindOnce([](PromiseResolver<int>* promise_resolver,
                  PromiseResolver<int> pr) { *promise_resolver = pr; },
               &promise_resolver));
  Promise<bool> p3 = Promise<bool>(BindOnce([](PromiseResolver<bool> pr) {}));

  promise::Race(p1, p2, p3)
      .Then(FROM_HERE,
            BindOnce(
                [](base::RunLoop* run_loop, Variant<float, int, bool> result) {
                  EXPECT_EQ(1337, Get<int>(&result));
                  run_loop->Quit();
                },
                &run_loop_));

  promise_resolver.resolve(1337);
  run_loop_.Run();
}

}  // namespace base
