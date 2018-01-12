// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_H_
#define BASE_PROMISE_PROMISE_H_

#include "base/promise/all_promise_executor.h"
#include "base/promise/promise_executors.h"
#include "base/promise/race_promise_executor.h"

namespace base {
class BASE_EXPORT promise;

template <typename ResolveType,
          typename RejectType =
              typename internal::PromiseTraits<ResolveType>::DefaultRejectType>
class BASE_EXPORT Promise {
 public:
  using ResolveT = ResolveType;
  using RejectT = RejectType;

  // Constructs an unresolved promise which much be manaually resolved via
  // GetResolver.
  Promise()
      : abstract_promise_(subtle::AdoptRefIfNeeded(
            new internal::AbstractPromise(
                internal::AbstractPromise::ConstructWith<ResolveType,
                                                         RejectType>(),
                nullopt,
                nullptr,
                internal::AbstractPromise::State::UNRESOLVED,
                internal::AbstractPromise::PrerequisitePolicy::NEVER,
                std::vector<scoped_refptr<internal::AbstractPromise>>(),
                nullptr),
            internal::AbstractPromise::kRefCountPreference)) {}

  // Returns a PromiseResolver<T> if this is an unresolved Promise().
  Optional<PromiseResolver<ResolveType, RejectType>> GetResolver() {
    DCHECK(abstract_promise_);
    if (!abstract_promise_->IsInitialPromise())
      return nullopt;
    return PromiseResolver<ResolveType, RejectType>(abstract_promise_);
  }

  // A task to execute |on_reject| is posted on |task_runner| as soon as the
  // this promise is rejected. A Promise<> for the return value of |on_reject|
  // is returned. Two reject types have special meaning:
  // 1. PromiseResult<Resolve, Reject> lets the callback resolve, reject or
  //    curry a Promise<Resolve, Reject>
  // 2. Promise<Resolve, Reject> where the result is a curried promise.
  template <typename RejectCb>
  typename internal::ComputeThenPromise<
      typename internal::CallbackTypeHelper<RejectCb>::ReturnT,
      RejectType>::Promise
  Catch(const Location& from_here, OnceCallback<RejectCb> on_reject) {
    DCHECK(abstract_promise_);
    using RejectReturnT =
        typename internal::CallbackTypeHelper<RejectCb>::ReturnT;
    using RejectArgT = typename internal::CallbackTypeHelper<RejectCb>::ArgT;

    static_assert(std::is_same<RejectArgT, RejectType>::value ||
                      std::is_void<RejectArgT>::value,
                  "The on_reject argument must be Promise::RejectType or void");

    using CatchPromise =
        typename internal::ComputeThenPromise<RejectReturnT,
                                              RejectType>::Promise;
    using ConstructWith =
        typename internal::ComputeThenPromise<RejectReturnT,
                                              RejectType>::ConstructWith;
    using ThenRejected =
        Rejected<typename internal::ComputeThenPromise<RejectReturnT,
                                                       RejectType>::ThenReject>;

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            ConstructWith(), from_here, nullptr,
            internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ALL,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                abstract_promise_},
            std::make_unique<internal::AbstractPromiseExecutor<
                OnceCallback<RejectReturnT()>, OnceCallback<RejectCb>, void,
                RejectArgT, ThenRejected>>(OnceCallback<RejectReturnT()>(),
                                           std::move(on_reject))),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return CatchPromise(std::move(promise));
  }

  // As soon as the this promise is resolved, a task to execute |on_resolve|
  // is posted on the ThreadTaskRunnerHandler. A Promise<> for the return
  // value of |on_resolve| is returned. Two reject types have special meaning:
  // 1. PromiseResult<Resolve, Reject> lets the callback resolve, reject or
  //    curry a Promise<Resolve, Reject>
  // 2. Promise<Resolve, Reject> where the result is a curried promise.
  template <typename ResolveCb>
  typename internal::ComputeThenPromise<
      typename internal::CallbackTypeHelper<ResolveCb>::ReturnT,
      RejectType>::Promise
  Then(const Location& from_here, OnceCallback<ResolveCb> on_resolve) {
    DCHECK(abstract_promise_);
    using ResolveReturnT =
        typename internal::CallbackTypeHelper<ResolveCb>::ReturnT;
    using ResolveArgT = typename internal::CallbackTypeHelper<ResolveCb>::ArgT;

    static_assert(
        std::is_same<ResolveArgT, ResolveType>::value ||
            std::is_void<ResolveArgT>::value,
        "The on_resolve argument must be Promise::ResolveType or void");

    using ThenPromise =
        typename internal::ComputeThenPromise<ResolveReturnT,
                                              RejectType>::Promise;
    using ConstructWith =
        typename internal::ComputeThenPromise<ResolveReturnT,
                                              RejectType>::ConstructWith;
    using ThenRejected =
        Rejected<typename internal::ComputeThenPromise<ResolveReturnT,
                                                       RejectType>::ThenReject>;

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            ConstructWith(), from_here, nullptr,
            internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ALL,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                abstract_promise_},
            std::make_unique<internal::AbstractPromiseExecutor<
                OnceCallback<ResolveCb>, OnceCallback<ResolveReturnT()>,
                ResolveArgT, void, ThenRejected>>(
                std::move(on_resolve), OnceCallback<ResolveReturnT()>())),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return ThenPromise(std::move(promise));
  }

  // As soon as the thus promise is resolved or rejected, a task to execute
  // |on_resolve| or |on_reject| is posted on the ThreadTaskRunnerHandle.  A
  // Promise<> for the return value of |on_resolve| or |on_reject| is returned.
  // Two reject types have special meaning:
  // 1. PromiseResult<Resolve, Reject> lets the callback resolve, reject or
  //    curry a Promise<Resolve, Reject>
  // 2. Promise<Resolve, Reject> where the result is a curried promise.
  template <typename ResolveCb, typename RejectCb>
  typename internal::ComputeThenPromise<
      typename internal::CallbackTypeHelper<ResolveCb>::ReturnT,
      RejectType>::Promise
  Then(const Location& from_here,
       OnceCallback<ResolveCb> on_resolve,
       OnceCallback<RejectCb> on_reject) {
    DCHECK(abstract_promise_);
    using ResolveReturnT =
        typename internal::CallbackTypeHelper<ResolveCb>::ReturnT;
    using RejectReturnT =
        typename internal::CallbackTypeHelper<RejectCb>::ReturnT;
    using ResolveArgT = typename internal::CallbackTypeHelper<ResolveCb>::ArgT;
    using RejectArgT = typename internal::CallbackTypeHelper<RejectCb>::ArgT;
    static_assert(std::is_same<ResolveReturnT, RejectReturnT>::value,
                  "Both callbacks must have the same return type");

    static_assert(
        std::is_same<ResolveArgT, ResolveType>::value ||
            std::is_void<ResolveArgT>::value,
        "The on_resolve argument must be Promise::ResolveType or void");

    static_assert(std::is_same<RejectArgT, RejectType>::value ||
                      std::is_void<RejectArgT>::value,
                  "The on_reject argument must be Promise::RejectType or void");

    using ThenPromise =
        typename internal::ComputeThenPromise<ResolveReturnT,
                                              RejectType>::Promise;
    using ConstructWith =
        typename internal::ComputeThenPromise<ResolveReturnT,
                                              RejectType>::ConstructWith;
    using ThenRejected =
        Rejected<typename internal::ComputeThenPromise<ResolveReturnT,
                                                       RejectType>::ThenReject>;

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            ConstructWith(), from_here, nullptr,
            internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ALL,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                abstract_promise_},
            std::make_unique<internal::AbstractPromiseExecutor<
                OnceCallback<ResolveCb>, OnceCallback<RejectCb>, ResolveArgT,
                RejectArgT, ThenRejected>>(std::move(on_resolve),
                                           std::move(on_reject))),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return ThenPromise(std::move(promise));
  }

  // A task to execute |finally_callback| is posted after the parent promise is
  // resolved or rejected and all Thens have executed.
  void Finally(const Location& from_here, OnceClosure finally_callback) {
    DCHECK(abstract_promise_);
    subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<void, void>(), from_here,
            nullptr, internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::FINALLY,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                abstract_promise_},
            std::make_unique<internal::FinallyExecutor>(
                std::move(finally_callback))),
        internal::AbstractPromise::kRefCountPreference);
  }

  // Cancels this promise and others that depend on it, with special handling of
  // promise::Race() to ensure that is only canceled if all prerequisites are
  // canceled.
  void Cancel() { abstract_promise_->Cancel(); }

 private:
  friend class promise;

  template <typename A, typename B>
  friend class Promise;

  template <typename Result, typename RejectStorage>
  friend struct internal::AssignHelper;

  template <typename Container, typename ContainerT>
  friend struct internal::AllContainerHelper;

  template <typename Container, typename ContainerT>
  friend struct internal::RaceContainerHelper;

  template <size_t N, typename...>
  friend struct internal::VariantPromiseHelper;

  explicit Promise(scoped_refptr<internal::AbstractPromise> abstract_promise)
      : abstract_promise_(abstract_promise) {}

  scoped_refptr<internal::AbstractPromise> abstract_promise_;
};

// Used for manually resolving and rejecting a Promise.
template <typename ResolveType,
          typename RejectType =
              typename internal::PromiseTraits<ResolveType>::DefaultRejectType>
class BASE_EXPORT PromiseResolver {
 public:
  PromiseResolver() {}

  explicit PromiseResolver(scoped_refptr<internal::AbstractPromise> promise)
      : promise_(std::move(promise)) {}

  template <typename... Ts>
  void Resolve(Ts... t) {
    bool success = promise_->value().TryAssign(
        Resolved<ResolveType>{std::forward<Ts>(t)...});
    DCHECK(success);
    promise_->OnManualResolve();
  }

  template <typename... Ts>
  void Reject(Ts... t) {
    bool success = promise_->value().TryAssign(
        Rejected<RejectType>{std::forward<Ts>(t)...});
    DCHECK(success);
    promise_->OnManualResolve();
  }

  scoped_refptr<internal::AbstractPromise> promise_;
};

// One of the supported promise result types. This allows a promise to
// dynamically reject or resolve (potentially with another promise).
template <typename ResolveType,
          typename RejectType =
              typename internal::PromiseTraits<ResolveType>::DefaultRejectType>
class BASE_EXPORT PromiseResult {
 public:
  using NonVoidResolveType =
      std::conditional_t<std::is_void<ResolveType>::value, Void, ResolveType>;
  using NonVoidRejectType =
      std::conditional_t<std::is_void<RejectType>::value, Void, RejectType>;

  // No argument constructor, can either resolve or reject depending on the
  // template arguments.
  template <typename ResolveT = ResolveType, typename RejectT = RejectType>
  PromiseResult(std::enable_if_t<!std::is_same<ResolveT, RejectT>::value &&
                                     (std::is_void<ResolveT>::value ||
                                      std::is_void<RejectT>::value),
                                 void>)
      : value_(std::conditional_t<std::is_void<ResolveT>::value,
                                  Resolved<void>,
                                  Rejected<void>>()) {}

  // Resolve overrides
  PromiseResult(const Resolved<ResolveType>& resolved) : value_(resolved) {}

  PromiseResult(Resolved<ResolveType>&& resolved)
      : value_(std::move(resolved)) {}

  template <typename ResolveT = ResolveType, typename RejectT = RejectType>
  PromiseResult(std::enable_if_t<!std::is_same<ResolveT, RejectT>::value &&
                                     !std::is_void<ResolveT>::value,
                                 const NonVoidResolveType&> resolved)
      : value_(Resolved<ResolveType>{resolved}) {}

  template <typename ResolveT = ResolveType, typename RejectT = RejectType>
  PromiseResult(std::enable_if_t<!std::is_same<ResolveT, RejectT>::value &&
                                     !std::is_void<ResolveT>::value,
                                 NonVoidResolveType&&> resolved)
      : value_(Resolved<ResolveType>{std::move(resolved)}) {}

  // Resolve with Curried promise
  PromiseResult(Promise<ResolveType, RejectType> curried_promise)
      : value_(std::move(curried_promise.internal_promise_)) {}

  // Reject overrides
  PromiseResult(const Rejected<RejectType>& rejected) : value_(rejected) {}

  PromiseResult(Rejected<RejectType>&& rejected)
      : value_(std::move(rejected)) {}

  template <typename ResolveT = ResolveType, typename RejectT = RejectType>
  PromiseResult(std::enable_if_t<!std::is_same<ResolveT, RejectT>::value &&
                                     !std::is_void<RejectT>::value,
                                 const NonVoidRejectType&> rejected)
      : value_(Rejected<RejectType>{rejected}) {}

  template <typename ResolveT = ResolveType, typename RejectT = RejectType>
  PromiseResult(std::enable_if_t<!std::is_same<ResolveT, RejectT>::value &&
                                     !std::is_void<RejectT>::value,
                                 NonVoidRejectType&&> rejected)
      : value_(Rejected<RejectType>{std::move(rejected)}) {}

  PromiseResult(PromiseResult&& other) = default;

  PromiseResult(const PromiseResult&) = delete;
  PromiseResult& operator=(const PromiseResult&) = delete;

  Variant<Resolved<ResolveType>,
          Rejected<RejectType>,
          scoped_refptr<internal::AbstractPromise>>&
  value() {
    return value_;
  }

  enum {
    kResolvedIndex = 0,
    kRejectedIndex = 1,
    kResolvedWithPromiseIndex = 2,
  };

 private:
  Variant<Resolved<ResolveType>,
          Rejected<RejectType>,
          scoped_refptr<internal::AbstractPromise>>
      value_;
};

class BASE_EXPORT promise {
 public:
  template <typename... Promises>
  struct VarArgIsPromise;

  /*
   * Accepts either a container of Promise<Resolve, Reject> and returns a
   * Promise<std::vector<Resolve>, Reject> or it accepts a container of
   * Variant<Promise<Resolve, Reject>...> and returns a
   * Promise<std::vector<Variant<Resolve...>>, Variant<Reject...>> which is
   * resolved when all input promises are resolved or any are rejected.
   */
  template <typename Container>
  static typename internal::
      AllContainerHelper<Container, typename Container::value_type>::PromiseType
      All(const Container& promises) {
    return internal::AllContainerHelper<
        Container, typename Container::value_type>::All(promises);
  }

  /*
   * Accepts one or more promises and returns a
   * Promise<std::tuple<Resolve> ...>, Variant<Reject...>> which is resolved
   * when all promises resolved or rejects with the RejectValue of the first
   * promise to do reject.
   */
  template <typename... Promises>
  static typename std::enable_if_t<
      VarArgIsPromise<Promises...>::value,
      typename internal::AllPromiseHelper<Promises...>>::ThenPromise
  All(Promises... promises) {
    using ThenResolve =
        typename internal::AllPromiseHelper<Promises...>::ThenResolve;
    using ThenReject =
        typename internal::AllPromiseHelper<Promises...>::ThenReject;
    using ThenPromise =
        typename internal::AllPromiseHelper<Promises...>::ThenPromise;

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ThenResolve, ThenReject>(),
            nullopt, nullptr, internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ALL,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                promises.abstract_promise_...},
            internal::AllPromiseExecutor::Create<ThenResolve>()),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return ThenPromise(std::move(promise));
  }

  /*
   * Accepts either a container of Promise<Resolve, Reject> and returns a
   * Promise<Resolve, Reject> or it accepts a container of
   * Variant<Promise<Resolve, Reject>...> and returns a
   * Promise<Variant<Resolve...>, Variant<Reject...>> which is resolved when any
   * input promise is resolved or rejected.
   */
  template <typename Container>
  static typename internal::RaceContainerHelper<
      Container,
      typename Container::value_type>::PromiseType
  Race(const Container& promises) {
    return internal::RaceContainerHelper<
        Container, typename Container::value_type>::Race(promises);
  }

  /*
   * Accepts one or more promises and returns a
   * Promise<Variant<distinct non-void promise types>> which is resolved
   * when any input promise is resolved or rejected.
   */
  template <typename... Promises>
  static typename std::enable_if_t<
      VarArgIsPromise<Promises...>::value,
      typename internal::RacePromiseHelper<Promises...>>::ThenPromise
  Race(Promises... promises) {
    using ThenResolve =
        typename internal::RacePromiseHelper<Promises...>::ThenResolve;
    using ThenReject =
        typename internal::RacePromiseHelper<Promises...>::ThenReject;
    using ThenPromise =
        typename internal::RacePromiseHelper<Promises...>::ThenPromise;

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ThenResolve, ThenReject>(),
            nullopt, nullptr, internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ANY,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                promises.abstract_promise_...},
            std::make_unique<internal::RacePromiseExecutor>()),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return ThenPromise(std::move(promise));
  }

  // Returns a rejected promise.
  template <typename ResolveType, typename RejectType>
  static Promise<ResolveType, RejectType> Reject(RejectType&& rejected) {
    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ResolveType, RejectType>(),
            nullopt, nullptr, internal::AbstractPromise::State::REJECTED,
            internal::AbstractPromise::PrerequisitePolicy::NEVER,
            std::vector<scoped_refptr<internal::AbstractPromise>>(), nullptr),
        internal::AbstractPromise::kRefCountPreference));
    bool success = promise->value().TryAssign(
        Rejected<RejectType>{std::forward<RejectType>(rejected)});
    DCHECK(success);
    return Promise<ResolveType, RejectType>(std::move(promise));
  }

  // Returns a rejected promise.
  template <typename ResolveType, typename>
  static Promise<ResolveType, void> Reject() {
    auto promise = subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ResolveType, void>(),
            nullopt, nullptr, internal::AbstractPromise::State::REJECTED,
            internal::AbstractPromise::PrerequisitePolicy::NEVER,
            std::vector<scoped_refptr<internal::AbstractPromise>>(), nullptr),
        internal::AbstractPromise::kRefCountPreference);
    bool success = promise->value().TryAssign(Rejected<void>());
    DCHECK(success);
    return Promise<ResolveType, void>(std::move(promise));
  }

  // Returns a resolved promise.
  template <typename ResolveType,
            typename RejectType = typename internal::PromiseTraits<
                ResolveType>::DefaultRejectType>
  static Promise<ResolveType, RejectType> Resolve(ResolveType&& resolved) {
    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ResolveType, RejectType>(),
            nullopt, nullptr, internal::AbstractPromise::State::RESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::NEVER,
            std::vector<scoped_refptr<internal::AbstractPromise>>(), nullptr),
        internal::AbstractPromise::kRefCountPreference));
    bool success = promise->value().TryAssign(
        Resolved<ResolveType>{std::forward<ResolveType>(resolved)});
    DCHECK(success);
    return Promise<ResolveType, RejectType>(std::move(promise));
  }

  template <typename RejectType =
                typename internal::PromiseTraits<void>::DefaultRejectType>
  static Promise<void, RejectType> Resolve() {
    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<void, RejectType>(),
            nullopt, nullptr, internal::AbstractPromise::State::RESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::NEVER,
            std::vector<scoped_refptr<internal::AbstractPromise>>(), nullptr),
        internal::AbstractPromise::kRefCountPreference));
    bool success = promise->value().TryAssign(Resolved<void>());
    DCHECK(success);
    return Promise<void, RejectType>(std::move(promise));
  }

 private:
  template <typename T>
  struct IsPromise {
    static constexpr bool value = false;
  };

  template <typename Resolve, typename Reject>
  struct IsPromise<Promise<Resolve, Reject>> {
    static constexpr bool value = true;
  };

  template <typename T, typename... Rest>
  struct VarArgIsPromise<T, Rest...> {
    static constexpr bool value = IsPromise<T>::value;
  };
};

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_H_
