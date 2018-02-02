// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_H_
#define BASE_PROMISE_PROMISE_H_

#include "base/promise/all_promise_executor.h"
#include "base/promise/promise_executors.h"
#include "base/promise/race_promise_executor.h"

namespace base {
class Promises;

template <typename ResolveType,
          typename RejectType =
              typename internal::PromiseTraits<ResolveType>::DefaultRejectType>
class Promise {
 public:
  // Constructs an unresolved promise for use by a ManualPromiseResolver<>.
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

  // Helpers for computing Then/Catch promise return and reject types based on
  // the callback signature.
  template <typename CB>
  using ThenResolve = typename internal::ComputeThenPromise<
      typename internal::CallbackTypeHelper<CB>::ReturnT,
      RejectType>::ThenResolve;

  template <typename CB>
  using ThenReject = typename internal::ComputeThenPromise<
      typename internal::CallbackTypeHelper<CB>::ReturnT,
      RejectType>::ThenReject;

  template <typename CB>
  using CallbackReturnType = typename internal::CallbackTypeHelper<CB>::ReturnT;

  template <typename CB>
  using CallbackArgumentType = typename internal::CallbackTypeHelper<CB>::ArgT;

  // A task to execute |on_reject| is posted on the current
  // SequencedTaskRunnerHandle as soon as the this promise is rejected. A
  // Promise<> for the return value of |on_reject| is returned. The following
  // callback return types have special meanings:
  // 1. PromiseResult<Resolve, Reject> lets the callback resolve, reject or
  //    curry a Promise<Resolve, Reject>
  // 2. Promise<Resolve, Reject> where the result is a curried promise.
  template <typename RejectCb>
  Promise<ThenResolve<RejectCb>, ThenReject<RejectCb>> Catch(
      const Location& from_here,
      OnceCallback<RejectCb> on_reject) {
    DCHECK(abstract_promise_);
    using ThenResolveT = ThenResolve<RejectCb>;
    using ThenRejectT = ThenReject<RejectCb>;
    using RejectReturnT = CallbackReturnType<RejectCb>;
    using RejectArgT = CallbackArgumentType<RejectCb>;

    static_assert(
        std::is_same<RejectArgT, RejectType>::value ||
            std::is_void<RejectArgT>::value,
        "|on_reject| callback must accept Promise::RejectType or void");

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ThenResolveT,
                                                     ThenRejectT>(),
            from_here, nullptr, internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ALL,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                abstract_promise_},
            std::make_unique<internal::AbstractPromiseExecutor<
                OnceCallback<RejectReturnT()>, OnceCallback<RejectCb>, void,
                RejectArgT, Rejected<ThenRejectT>>>(
                OnceCallback<RejectReturnT()>(), std::move(on_reject))),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return Promise<ThenResolveT, ThenRejectT>(std::move(promise));
  }

  // As soon as the this promise is resolved, a task to execute |on_resolve|
  // is posted on the current SequencedTaskRunnerHandle. A Promise<> for the
  // return value of |on_resolve| is returned. The following callback return
  // types have special meanings:
  // 1. PromiseResult<Resolve, Reject> lets the callback resolve, reject or
  //    curry a Promise<Resolve, Reject>
  // 2. Promise<Resolve, Reject> where the result is a curried promise.
  template <typename ResolveCb>
  Promise<ThenResolve<ResolveCb>, ThenReject<ResolveCb>> Then(
      const Location& from_here,
      OnceCallback<ResolveCb> on_resolve) {
    DCHECK(abstract_promise_);
    using ThenResolveT = ThenResolve<ResolveCb>;
    using ThenRejectT = ThenReject<ResolveCb>;
    using ResolveReturnT = CallbackReturnType<ResolveCb>;
    using ResolveArgT = CallbackArgumentType<ResolveCb>;
    static_assert(
        std::is_same<ResolveArgT, ResolveType>::value ||
            std::is_void<ResolveArgT>::value,
        "|on_resolve| callback must accept Promise::ResolveType or void");

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ThenResolveT,
                                                     ThenRejectT>(),
            from_here, nullptr, internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ALL,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                abstract_promise_},
            std::make_unique<internal::AbstractPromiseExecutor<
                OnceCallback<ResolveCb>, OnceCallback<ResolveReturnT()>,
                ResolveArgT, void, Rejected<ThenRejectT>>>(
                std::move(on_resolve), OnceCallback<ResolveReturnT()>())),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return Promise<ThenResolveT, ThenRejectT>(std::move(promise));
  }

  // As soon as the this promise is resolved or rejected, a task to execute
  // |on_resolve| or |on_reject| is posted on the current
  // SequencedTaskRunnerHandle. A Promise<> for the return value of
  // |on_resolve| or |on_reject| is returned. The following callback return
  // types have special meanings:
  // 1. PromiseResult<Resolve, Reject> lets the callback resolve, reject or
  //    curry a Promise<Resolve, Reject>
  // 2. Promise<Resolve, Reject> where the result is a curried promise.
  template <typename ResolveCb, typename RejectCb>
  Promise<ThenResolve<ResolveCb>, ThenReject<ResolveCb>> Then(
      const Location& from_here,
      OnceCallback<ResolveCb> on_resolve,
      OnceCallback<RejectCb> on_reject) {
    DCHECK(abstract_promise_);
    using ThenResolveT = ThenResolve<ResolveCb>;
    using ThenRejectT = ThenReject<ResolveCb>;
    using ResolveReturnT = CallbackReturnType<ResolveCb>;
    using ResolveArgT = CallbackArgumentType<ResolveCb>;
    using RejectReturnT = CallbackReturnType<RejectCb>;
    using RejectArgT = CallbackArgumentType<RejectCb>;

    static_assert(std::is_same<ResolveReturnT, RejectReturnT>::value,
                  "Both callbacks must have the same return type");
    static_assert(
        std::is_same<ResolveArgT, ResolveType>::value ||
            std::is_void<ResolveArgT>::value,
        "|on_resolve| callback must accept Promise::ResolveType or void");

    static_assert(
        std::is_same<RejectArgT, RejectType>::value ||
            std::is_void<RejectArgT>::value,
        "|on_reject| callback must accept Promise::RejectType or void");

    scoped_refptr<internal::AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new internal::AbstractPromise(
            internal::AbstractPromise::ConstructWith<ThenResolveT,
                                                     ThenRejectT>(),
            from_here, nullptr, internal::AbstractPromise::State::UNRESOLVED,
            internal::AbstractPromise::PrerequisitePolicy::ALL,
            std::vector<scoped_refptr<internal::AbstractPromise>>{
                abstract_promise_},
            std::make_unique<internal::AbstractPromiseExecutor<
                OnceCallback<ResolveCb>, OnceCallback<RejectCb>, ResolveArgT,
                RejectArgT, Rejected<ThenRejectT>>>(std::move(on_resolve),
                                                    std::move(on_reject))),
        internal::AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return Promise<ThenResolveT, ThenRejectT>(std::move(promise));
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

  using ResolveT = ResolveType;
  using RejectT = RejectType;

 private:
  friend class Promises;

  template <typename A, typename B>
  friend class Promise;

  template <typename A, typename B>
  friend class PromiseResult;

  template <typename Result, typename RejectStorage>
  friend struct internal::AssignHelper;

  template <typename Container, typename ContainerT>
  friend struct internal::AllContainerHelper;

  template <typename Container, typename ContainerT>
  friend struct internal::RaceContainerHelper;

  template <size_t N, typename...>
  friend struct internal::VariantPromiseHelper;

  template <typename A, typename B>
  friend class ManualPromiseResolver;

  explicit Promise(scoped_refptr<internal::AbstractPromise> abstract_promise)
      : abstract_promise_(abstract_promise) {}

  scoped_refptr<internal::AbstractPromise> abstract_promise_;
};

// Used for manually resolving and rejecting a Promise. This is for
// compatibility with old code and will eventually be removed.
template <typename ResolveType,
          typename RejectType =
              typename internal::PromiseTraits<ResolveType>::DefaultRejectType>
class ManualPromiseResolver {
 public:
  ManualPromiseResolver() {}

  template <typename... Ts>
  void Resolve(Ts... t) {
    bool success = promise_.abstract_promise_->value().TryAssign(
        Resolved<ResolveType>{std::forward<Ts>(t)...});
    DCHECK(success);
    promise_.abstract_promise_->OnManualResolve();
  }

  template <typename... Ts>
  void Reject(Ts... t) {
    bool success = promise_.abstract_promise_->value().TryAssign(
        Rejected<RejectType>{std::forward<Ts>(t)...});
    DCHECK(success);
    promise_.abstract_promise_->OnManualResolve();
  }

  typename internal::PromiseCallbackHelper<ResolveType>::Callback
  GetResolveCallback() {
    return internal::PromiseCallbackHelper<ResolveType>::GetResolveCallback(
        promise_.abstract_promise_);
  }

  typename internal::PromiseCallbackHelper<RejectType>::Callback
  GetRejectCallback() {
    return internal::PromiseCallbackHelper<RejectType>::GetRejectCallback(
        promise_.abstract_promise_);
  }

  Promise<ResolveType, RejectType> promise() const { return promise_; }

 private:
  Promise<ResolveType, RejectType> promise_;
};

// One of the supported promise result types. This allows a promise to
// dynamically reject or resolve (potentially with another promise).
template <typename ResolveType,
          typename RejectType =
              typename internal::PromiseTraits<ResolveType>::DefaultRejectType>
class PromiseResult {
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
      : value_(std::move(curried_promise.abstract_promise_)) {}

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

class Promises {
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
  template <typename... Resolve, typename... Reject>
  static Promise<std::tuple<internal::ToNonVoidT<Resolve>...>,
                 typename internal::UnionOfVarArgTypes<Reject...>::type>
  All(Promise<Resolve, Reject>... promises) {
    using ThenResolve = std::tuple<internal::ToNonVoidT<Resolve>...>;
    using ThenReject = typename internal::UnionOfVarArgTypes<Reject...>::type;
    using ThenPromise = Promise<ThenResolve, ThenReject>;

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
  template <typename... Resolve, typename... Reject>
  static Promise<typename internal::UnionOfVarArgTypes<Resolve...>::type,
                 typename internal::UnionOfVarArgTypes<Reject...>::type>
  Race(Promise<Resolve, Reject>... promises) {
    using ThenResolve = typename internal::UnionOfVarArgTypes<Resolve...>::type;
    using ThenReject = typename internal::UnionOfVarArgTypes<Reject...>::type;
    using ThenPromise = Promise<ThenResolve, ThenReject>;

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
