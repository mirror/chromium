// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_RACE_PROMISE_EXECUTOR_H_
#define BASE_PROMISE_RACE_PROMISE_EXECUTOR_H_

#include "base/promise/abstract_promise.h"

namespace base {
namespace internal {

class BASE_EXPORT RacePromiseExecutor : public AbstractPromise::Executor {
 public:
  RacePromiseExecutor();
  ~RacePromiseExecutor() override;

  void Execute(AbstractPromise* promise) override;
};

// Helper for constructing Race promises.
template <typename Container, typename ContainerT>
struct RaceContainerHelper;

template <typename Container, typename Resolve, typename Reject>
struct RaceContainerHelper<Container, Promise<Resolve, Reject>> {
  using PromiseType = Promise<Resolve, Reject>;

  static PromiseType Race(const Container& promises) {
    std::vector<scoped_refptr<AbstractPromise>> prerequistes;
    prerequistes.reserve(promises.size());
    for (typename Container::const_iterator it = promises.begin();
         it != promises.end(); ++it) {
      prerequistes.push_back(it->abstract_promise_);
    }
    scoped_refptr<AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new AbstractPromise(
            AbstractPromise::ConstructWith<Resolve, Reject>(), nullopt, nullptr,
            AbstractPromise::State::UNRESOLVED,
            AbstractPromise::PrerequisitePolicy::ANY, std::move(prerequistes),
            std::make_unique<RacePromiseExecutor>()),
        AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return PromiseType(std::move(promise));
  }
};

template <typename Container, typename... Promises>
struct RaceContainerHelper<Container, Variant<Promises...>> {
  using PromiseResolve =
      typename UnionOfVarArgTypes<typename Promises::ResolveT...>::type;
  using PromiseReject =
      typename UnionOfVarArgTypes<typename Promises::RejectT...>::type;
  using PromiseType = Promise<PromiseResolve, PromiseReject>;

  static PromiseType Race(const Container& promises) {
    std::vector<scoped_refptr<AbstractPromise>> prerequistes;
    prerequistes.reserve(promises.size());
    for (typename Container::const_iterator it = promises.begin();
         it != promises.end(); ++it) {
      prerequistes.push_back(
          VariantPromiseHelper<0, Promises...>::GetAbstractPromise(*it));
    }
    scoped_refptr<AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new AbstractPromise(
            AbstractPromise::ConstructWith<PromiseResolve, PromiseReject>(),
            nullopt, nullptr, AbstractPromise::State::UNRESOLVED,
            AbstractPromise::PrerequisitePolicy::ANY, std::move(prerequistes),
            std::make_unique<RacePromiseExecutor>()),
        AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return PromiseType(std::move(promise));
  }
};
}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_RACE_PROMISE_EXECUTOR_H_
