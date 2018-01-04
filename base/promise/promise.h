// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_H_
#define BASE_PROMISE_PROMISE_H_

#include "base/promise/promise_internal.h"

namespace base {
class BASE_EXPORT promise;

// base::Promise is inspired by ES6 promises and has more or less the same API.
// Because C++ isn't a dynamic language (std::any not withstanding) there are
// some practical differences:
// 1. base::Value is used for reject reason. While it's possible to templateize
//    the reject type, we'd end up having to store the reject reason in a
//    std::any to cope with chained Thens.
// 2. promise::Race returns a de-duped base::Variant<> of the resolve types
//    where void is renamed Void.
// 3. promise::All returns a std::tuple<T...> where void is renamed Void.
//
// The function passed into the Then(), Catch() or Finally() may either run
// immediately when eligible, or trigger a PostTask (variants with FROM_HERE)
// with an option sequence task runner specified.
//
// Then() and Catch() accept base::Callbacks with the following return types:
// 1. Type where the Then/Catch returns a Promise<Type> which never gets
//    rejected.
// 2. Promise<Type> where the Then/Catch returns a Promise<Type> which curries
//    the returned promise.
// 3. PromiseResult<Type> where the Then/Catch can dynamically choose to
//    resolve, reject or curry a promise.
//
// The callback for Then() either accepts the promise value or void. E.g.
// Promise<int> p1;
// Promise<int> p2 = p1.Then(BindOnce([](int result) { return result + 1; }));
// Where p2 returns the value of p1 plus one.
//
// Promise<void> p1;
// Promise<void> p2;
// Promise<void> p3;
// promise::All(p1, p2, p3).Then(BindOnce([]() {
//    // Do something when p1, p2 & p3 are resolved
// }));
template <typename T>
class BASE_EXPORT Promise {
 public:
  // An unresolved promise which can be manually resolved via GetResolver().
  Promise() {
    internal_promise_ = MakeRefCounted<internal::InitialPromise<T>>();
  }

  // Returns a PromiseResolver<T> if available. Only an initial Promise<T>()
  // supports these.
  Optional<PromiseResolver<T>> GetResolver() {
    return internal_promise_->GetResolver();
  }

  // A hack to prevent Catch() and Then() failing to compile when T is void.  We
  // don't expect Catch<Void>(...) or Then<Void>(...) to be used (although they
  // should work).
  using NonVoidT = std::conditional_t<std::is_void<T>::value, Void, T>;

  // As soon as the parent promise is resolved or rejected, a task to execute
  // |on_resolve| or |on_reject| is posted on either the ThreadTaskRunnerHandle
  // or on any previously specified task runner. A Promise<R> for the return
  // value of |on_resolve| or |on_reject| is returned.  Three signatures are
  // supported for the return type of |on_resolve| or |on_reject|:
  // 1. PromiseResult<R> which supports resolve, reject and resolve with a
  //    curried Promise<R>.
  // 2. Promise<R> where the result is a curried promise.
  // 3. R
  template <typename Result>
  internal::PromiseReturnType<Result> Then(
      const Location& from_here,
      OnceCallback<Result()> on_resolve,
      OnceCallback<Result(const RejectValue&)> on_reject =
          OnceCallback<Result(const RejectValue&)>()) {
    DCHECK(internal_promise_);
    auto promise = ThenInternalVoidT(from_here, nullptr, std::move(on_resolve),
                                     std::move(on_reject));
    promise.internal_promise_->internal::PromiseBase::ExecuteIfPossible();
    return promise;
  }
  template <typename Result>
  internal::PromiseReturnType<Result> Then(
      const Location& from_here,
      OnceCallback<Result(NonVoidT)> on_resolve,
      OnceCallback<Result(const RejectValue&)> on_reject =
          OnceCallback<Result(const RejectValue&)>()) {
    DCHECK(internal_promise_);
    auto promise = ThenInternal(from_here, nullptr, std::move(on_resolve),
                                std::move(on_reject));
    promise.internal_promise_->internal::PromiseBase::ExecuteIfPossible();
    return promise;
  }

  // A task to execute |on_resolve| or |on_reject| is posted on |task_runner| as
  // soon as the parent promise is resolved or rejected. A Promise<R> for the
  // return value of |on_resolve| or |on_reject| is returned.  Three signatures
  // are supported for the return type of |on_resolve| or |on_reject|:
  // 1. PromiseResult<R> which supports resolve, reject and resolve with a
  //    curried Promise<R>.
  // 2. Promise<R> where the result is a curried promise.
  // 3. R
  template <typename Result>
  internal::PromiseReturnType<Result> Then(
      const Location& from_here,
      scoped_refptr<SequencedTaskRunner>& task_runner,
      OnceCallback<Result()> on_resolve,
      OnceCallback<Result(const RejectValue&)> on_reject =
          OnceCallback<Result(const RejectValue&)>()) {
    DCHECK(internal_promise_);
    auto promise = ThenInternalVoidT(
        from_here, task_runner, std::move(on_resolve), std::move(on_reject));
    promise.internal_promise_->internal::PromiseBase::ExecuteIfPossible();
    return promise;
  }
  template <typename Result>
  internal::PromiseReturnType<Result> Then(
      const Location& from_here,
      scoped_refptr<SequencedTaskRunner>& task_runner,
      OnceCallback<Result(NonVoidT)> on_resolve,
      OnceCallback<Result(const RejectValue&)> on_reject =
          OnceCallback<Result(const RejectValue&)>()) {
    DCHECK(internal_promise_);
    auto promise = ThenInternal(from_here, task_runner, std::move(on_resolve),
                                std::move(on_reject));
    promise.internal_promise_->internal::PromiseBase::ExecuteIfPossible();
    return promise;
  }

  // As soon as the parent promise is rejected, a task to execute |on_reject| is
  // posted on either the ThreadTaskRunnerHandle or on any previously specified
  // task runner. A Promise<R> for the return value of |on_reject| is returned.
  // Three signatures are supported
  // for the return type of |on_resolve| or |on_reject|:
  // 1. PromiseResult<R> which supports resolve, reject and resolve with a
  //    curried Promise<R>.
  // 2. Promise<R> where the result is a curried promise.
  // 3. R
  template <typename Result>
  internal::PromiseReturnType<Result> Catch(
      const Location& from_here,
      OnceCallback<Result(const RejectValue&)> on_reject) {
    DCHECK(internal_promise_);
    auto promise =
        ThenInternal(from_here, nullptr, OnceCallback<Result(NonVoidT)>(),
                     std::move(on_reject));
    promise.internal_promise_->internal::PromiseBase::ExecuteIfPossible();
    return promise;
  }

  // A task to execute |on_reject| is posted on |task_runner| as soon as the
  // parent promise is rejected  A Promise<R> for the return value of
  // |on_reject| is returned. Three signatures are supported for the return type
  // of |on_reject|:
  // 1. PromiseResult<R> which supports resolve, reject and resolve with a
  //    curried Promise<R>.
  // 2. Promise<R> where the result is a curried promise.
  // 3. R
  template <typename Result>
  internal::PromiseReturnType<Result> Catch(
      const Location& from_here,
      scoped_refptr<SequencedTaskRunner>& task_runner,
      OnceCallback<Result(const RejectValue&)> on_reject) {
    DCHECK(internal_promise_);
    auto promise =
        ThenInternal(from_here, task_runner, OnceCallback<Result(NonVoidT)>(),
                     std::move(on_reject));
    promise.internal_promise_->internal::PromiseBase::ExecuteIfPossible();
    return promise;
  }

  // |finally_callback| is called after the parent promise is resolved or
  // rejected and all Thens have executed.
  void Finally(OnceClosure finally_callback) {
    DCHECK(internal_promise_);
    // internal::FinallyPromise will auto delete.
    MakeRefCounted<internal::FinallyPromise>(
        nullopt, nullptr, internal_promise_, std::move(finally_callback))
        ->internal::PromiseBase::ExecuteIfPossible();
  }

  // A task to execute |finally_callback| is posted after the parent promise is
  // resolved or rejected and all Thens have executed.
  void Finally(const Location& from_here, OnceClosure finally_callback) {
    DCHECK(internal_promise_);
    // internal::FinallyPromise will auto delete.
    MakeRefCounted<internal::FinallyPromise>(
        from_here, nullptr, internal_promise_, std::move(finally_callback))
        ->internal::PromiseBase::ExecuteIfPossible();
  }
  void Finally(const Location& from_here,
               scoped_refptr<SequencedTaskRunner>& task_runner,
               OnceClosure finally_callback) {
    DCHECK(internal_promise_);
    // internal::FinallyPromise will auto delete.
    MakeRefCounted<internal::FinallyPromise>(from_here, std::move(task_runner),
                                             internal_promise_,
                                             std::move(finally_callback))
        ->internal::PromiseBase::ExecuteIfPossible();
  }

  // Cancels this promise and others that depend on it, with special handling of
  // promise::Race() to ensure that is only canceled if all prerequisites are
  // canceled.
  void Cancel() {
    internal_promise_->Cancel();
    internal_promise_ = nullptr;
  }

 private:
  explicit Promise(scoped_refptr<internal::PromiseT<T>> internal_promise)
      : internal_promise_(internal_promise) {}

  template <typename TT, typename ArgT>
  friend class internal::CurringPromise;

  template <size_t N, typename...>
  friend struct internal::VariantPromiseHelper;

  template <typename TT>
  friend struct internal::ContainerPromiseHelper;

  template <typename TT>
  friend class Promise;

  template <typename TT>
  friend class PromiseResult;

  friend class promise;

  template <typename R>
  Promise<R> ThenInternal(
      const Location& from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<PromiseResult<R>(NonVoidT)> on_resolve,
      OnceCallback<PromiseResult<R>(const RejectValue&)> on_reject =
          OnceCallback<PromiseResult<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<internal::ChainedPromise<R, NonVoidT>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternal(
      const Location& from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<Promise<R>(NonVoidT)> on_resolve,
      OnceCallback<Promise<R>(const RejectValue&)> on_reject =
          OnceCallback<Promise<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<internal::CurringPromise<R, NonVoidT>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternal(const Location& from_here,
                          scoped_refptr<SequencedTaskRunner> task_runner,
                          OnceCallback<R(NonVoidT)> on_resolve,
                          OnceCallback<R(const RejectValue&)> on_reject =
                              OnceCallback<R(const RejectValue&)>()) {
    return Promise<R>(
        MakeRefCounted<internal::NoRejectChainedPromise<R, NonVoidT>>(
            from_here, task_runner, internal_promise_, std::move(on_resolve),
            std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternalVoidT(
      const Location& from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<PromiseResult<R>()> on_resolve,
      OnceCallback<PromiseResult<R>(const RejectValue&)> on_reject =
          OnceCallback<PromiseResult<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<internal::ChainedPromise<R, void>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternalVoidT(
      const Location& from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<Promise<R>()> on_resolve,
      OnceCallback<Promise<R>(const RejectValue&)> on_reject =
          OnceCallback<Promise<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<internal::CurringPromise<R, void>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternalVoidT(const Location& from_here,
                               scoped_refptr<SequencedTaskRunner> task_runner,
                               OnceCallback<R()> on_resolve,
                               OnceCallback<R(const RejectValue&)> on_reject =
                                   OnceCallback<R(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<internal::NoRejectChainedPromise<R, void>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  scoped_refptr<internal::PromiseT<T>> internal_promise_;
};

class BASE_EXPORT promise {
 public:
  /*
   * Accepts one or more promises and returns a
   * Promise<std::tuple<PromiseType> ...>> which is resolved when all promises
   * resolved or rejects with the RejectValue of the first promise to do reject.
   */
  template <typename... Promises>
  static auto All(Promises... promises) {
    using ReturnType = std::tuple<
        typename internal::UnwrapPromise<Promises>::non_void_type...>;
    return Promise<ReturnType>(
        MakeRefCounted<internal::AllPromise<ReturnType, Promises...>>(
            std::vector<scoped_refptr<internal::PromiseBase>>(
                {GetPromiseBase(promises)...})));
  }

  /*
   * Accepts either a container of promises and returns a
   * Promise<std::vector<Container::value_type>>> or it accepts a container of
   * Variant<Promise<Ts>...> and returns a Promise<Variant<Ts...>> which is
   * resolved when any input promise is resolved or rejected.
   */
  template <typename Container>
  static auto All(const Container& promises) {
    using ContainerT = typename Container::value_type;
    std::vector<scoped_refptr<internal::PromiseBase>> prerequistes;
    std::vector<size_t> prerequisite_variant_indices;
    prerequistes.reserve(promises.size());
    prerequisite_variant_indices.reserve(promises.size());
    for (typename Container::const_iterator it = promises.begin();
         it != promises.end(); ++it) {
      prerequistes.push_back(
          internal::ContainerPromiseHelper<ContainerT>::GetPromiseBase(*it));
      prerequisite_variant_indices.push_back(
          internal::ContainerPromiseHelper<ContainerT>::GetPromiseVariantIndex(
              *it));
    }
    using ReturnType =
        typename internal::ContainerPromiseHelper<ContainerT>::ReturnType;
    return Promise<std::vector<ReturnType>>(
        MakeRefCounted<internal::ContainerAllPromise<ContainerT>>(
            std::move(prerequistes), std::move(prerequisite_variant_indices)));
  }

  /*
   * Accepts one or more promises and returns a
   * Promise<Variant<distinct non-void promise types>> which is resolved
   * when any input promise is resolved or rejected.
   */
  template <typename... Promises>
  static auto Race(Promises... promises) {
    using ReturnType = typename internal::DedupVariant<
        typename internal::UnwrapPromise<Promises>::non_void_type...>::type;
    return Promise<ReturnType>(
        MakeRefCounted<internal::RacePromise<ReturnType, Promises...>>(
            std::vector<scoped_refptr<internal::PromiseBase>>(
                {GetPromiseBase(promises)...})));
  }

  /*
   * Accepts either a container of promises and returns a
   * Promise<Container::value_type> or it accepts a container of
   * Variant<Promise<Ts>...> and returns a Promise<Variant<Ts...>> which is
   * resolved when any input promise is resolved or rejected.
   */
  template <typename Container>
  static auto Race(const Container& promises) {
    using ContainerT = typename Container::value_type;
    std::vector<scoped_refptr<internal::PromiseBase>> prerequistes;
    std::vector<size_t> prerequisite_variant_indices;
    prerequistes.reserve(promises.size());
    prerequisite_variant_indices.reserve(promises.size());
    for (typename Container::const_iterator it = promises.begin();
         it != promises.end(); ++it) {
      prerequistes.push_back(
          internal::ContainerPromiseHelper<ContainerT>::GetPromiseBase(*it));
      prerequisite_variant_indices.push_back(
          internal::ContainerPromiseHelper<ContainerT>::GetPromiseVariantIndex(
              *it));
    }
    using ReturnType =
        typename internal::ContainerPromiseHelper<ContainerT>::ReturnType;
    return Promise<ReturnType>(
        MakeRefCounted<internal::ContainerRacePromise<ContainerT>>(
            std::move(prerequistes), std::move(prerequisite_variant_indices)));
  }

  /* Returns a rejected promise.*/
  template <typename T>
  static Promise<T> Reject(Value err) {
    return Promise<T>(
        MakeRefCounted<internal::PromiseT<T>>(RejectValue(std::move(err))));
  }

  /* Returns a resolved promise.*/
  template <typename T>
  static Promise<T> Resolve(T&& t) {
    return Promise<T>(
        MakeRefCounted<internal::PromiseT<T>>(std::forward<T>(t)));
  }

  static Promise<void> Resolve() {
    return Promise<void>(MakeRefCounted<internal::PromiseT<void>>(Void()));
  }

 private:
  template <typename Promise>
  static scoped_refptr<internal::PromiseBase> GetPromiseBase(Promise p) {
    return p.internal_promise_;
  }

  template <typename Promise>
  static auto& GetPromiseValueRef(Promise p) {
    return p.internal_promise_->value();
  }
};

template <typename T>
void PromiseResolver<T>::Resolve(T&& t) {
  promise_->ResolveInternal(std::forward<T>(t));
}

template <typename T>
void PromiseResolver<T>::Reject(RejectValue err) {
  promise_->RejectInternal(std::move(err));
}

void PromiseResolver<void>::Resolve() {
  promise_->ResolveInternal();
}

void PromiseResolver<void>::Reject(RejectValue err) {
  promise_->RejectInternal(std::move(err));
}

template <typename... Args>
void PromiseResolver<void>::Reject(Args... args) {
  promise_->RejectInternal(RejectValue(std::forward<Args>(args)...));
}

// One of the supported promise result types. This allows a promise to
// dynamically reject or resolve (potentially with another promise).
template <typename T>
class BASE_EXPORT PromiseResult {
 public:
  using NonVoidT =
      typename std::conditional<std::is_void<T>::value, Void, T>::type;

  PromiseResult(RejectValue err) : value_(std::move(err)) {}

  PromiseResult(Promise<T> chained_promise)
      : value_(std::move(chained_promise.internal_promise_)) {}

  template <typename Arg>
  PromiseResult(Arg&& arg)
      : value_(in_place_index_t<0>(), std::forward<Arg>(arg)) {}

  PromiseResult(PromiseResult&& other) = default;

  PromiseResult(const PromiseResult&) = delete;
  PromiseResult& operator=(const PromiseResult&) = delete;

  Variant<NonVoidT, RejectValue, scoped_refptr<internal::PromiseT<T>>>&
  value() {
    return value_;
  }

  enum {
    kResolvedIndex = 0,
    kRejectedIndex = 1,
    kResolvedWithPromiseIndex = 2,
  };

 private:
  Variant<NonVoidT, RejectValue, scoped_refptr<internal::PromiseT<T>>> value_;
};

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_H_
