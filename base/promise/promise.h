// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_H_
#define BASE_PROMISE_PROMISE_H_

#include "base/promise/promise_internal.h"

namespace base {
struct promise;

template <typename T, typename ArgT>
class CurringPromise;

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
  Promise(OnceCallback<void(PromiseResolver<T>)> initalizer) {
    scoped_refptr<InitialPromise<T>> promise(new InitialPromise<T>());
    std::move(initalizer).Run(PromiseResolver<T>(promise));
    internal_promise_ = promise;
  }

  explicit Promise(scoped_refptr<InternalPromise<T>> internal_promise)
      : internal_promise_(internal_promise) {}

  // A hack to prevent Catch() and Then() failing to compile when T is void.  We
  // don't expect Catch<Void>(...) or Then<Void>(...) to be used (although they
  // should work).
  using NonVoidT = std::conditional_t<std::is_void<T>::value, Void, T>;

  // |on_reject| is executed as soon as the parent promise is rejected. A
  // Promise<R> for the return value of |on_reject| is returned. Three
  // signatures are supported for the return type of |on_reject|: 1.
  // PromiseResult<R> which supports resolve, reject and resolve with a
  //    curried Promise<R>.
  // 2. Promise<R> where the result is a curried promise.
  // 3. R
  template <typename Result>
  auto Catch(OnceCallback<Result(const RejectValue&)> on_reject) {
    auto promise =
        ThenInternal(nullopt, nullptr, OnceCallback<Result(NonVoidT)>(),
                     std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
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
  auto Catch(const Location& from_here,
             OnceCallback<Result(const RejectValue&)> on_reject) {
    auto promise =
        ThenInternal(from_here, nullptr, OnceCallback<Result(NonVoidT)>(),
                     std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
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
  auto Catch(const Location& from_here,
             scoped_refptr<SequencedTaskRunner>& task_runner,
             OnceCallback<Result(const RejectValue&)> on_reject) {
    auto promise =
        ThenInternal(from_here, task_runner, OnceCallback<Result(NonVoidT)>(),
                     std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
    return promise;
  }

  // |on_resolve| or |on_reject| are executed as soon as the parent promise
  // is resolved or rejected. A Promise<R> for the return value of |on_resolve|
  // or |on_reject| is returned.  Three signatures are supported for the return
  // type of |on_resolve| or |on_reject|:
  // 1. PromiseResult<R> which supports resolve, reject and resolve with a
  //    curried Promise<R>.
  // 2. Promise<R> where the result is a curried promise.
  // 3. R
  template <typename Result>
  auto Then(OnceCallback<Result()> on_resolve,
            OnceCallback<Result(const RejectValue&)> on_reject =
                OnceCallback<Result(const RejectValue&)>()) {
    auto promise = ThenInternalVoidT(nullopt, nullptr, std::move(on_resolve),
                                     std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
    return promise;
  }
  template <typename Result>
  auto Then(OnceCallback<Result(NonVoidT)> on_resolve,
            OnceCallback<Result(const RejectValue&)> on_reject =
                OnceCallback<Result(const RejectValue&)>()) {
    auto promise = ThenInternal(nullopt, nullptr, std::move(on_resolve),
                                std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
    return promise;
  }

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
  auto Then(const Location& from_here,
            OnceCallback<Result()> on_resolve,
            OnceCallback<Result(const RejectValue&)> on_reject =
                OnceCallback<Result(const RejectValue&)>()) {
    auto promise = ThenInternalVoidT(from_here, nullptr, std::move(on_resolve),
                                     std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
    return promise;
  }
  template <typename Result>
  auto Then(const Location& from_here,
            OnceCallback<Result(NonVoidT)> on_resolve,
            OnceCallback<Result(const RejectValue&)> on_reject =
                OnceCallback<Result(const RejectValue&)>()) {
    auto promise = ThenInternal(from_here, nullptr, std::move(on_resolve),
                                std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
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
  auto Then(const Location& from_here,
            scoped_refptr<SequencedTaskRunner>& task_runner,
            OnceCallback<Result()> on_resolve,
            OnceCallback<Result(const RejectValue&)> on_reject =
                OnceCallback<Result(const RejectValue&)>()) {
    auto promise = ThenInternalVoidT(
        from_here, task_runner, std::move(on_resolve), std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
    return promise;
  }
  template <typename Result>
  auto Then(const Location& from_here,
            scoped_refptr<SequencedTaskRunner>& task_runner,
            OnceCallback<Result(NonVoidT)> on_resolve,
            OnceCallback<Result(const RejectValue&)> on_reject =
                OnceCallback<Result(const RejectValue&)>()) {
    auto promise = ThenInternal(from_here, task_runner, std::move(on_resolve),
                                std::move(on_reject));
    promise.internal_promise_->PromiseBase::ExecuteIfPossible();
    return promise;
  }

  // |finally_callback| is called after the parent promise is resolved or
  // rejected and all Thens have executed.
  void Finally(OnceClosure finally_callback) {
    // FinallyPromise will auto delete.
    MakeRefCounted<FinallyPromise>(nullopt, nullptr, internal_promise_,
                                   std::move(finally_callback))
        ->PromiseBase::ExecuteIfPossible();
  }

  // A task to execute |finally_callback| is posted after the parent promise is
  // resolved or rejected and all Thens have executed.
  void Finally(const Location& from_here, OnceClosure finally_callback) {
    // FinallyPromise will auto delete.
    MakeRefCounted<FinallyPromise>(from_here, nullptr, internal_promise_,
                                   std::move(finally_callback))
        ->PromiseBase::ExecuteIfPossible();
  }
  void Finally(const Location& from_here,
               scoped_refptr<SequencedTaskRunner>& task_runner,
               OnceClosure finally_callback) {
    // FinallyPromise will auto delete.
    MakeRefCounted<FinallyPromise>(from_here, std::move(task_runner),
                                   internal_promise_,
                                   std::move(finally_callback))
        ->PromiseBase::ExecuteIfPossible();
  }

 private:
  template <typename TT, typename ArgT>
  friend class CurringPromise;

  template <typename TT>
  friend class Promise;

  template <typename TT>
  friend struct PromiseResult;

  friend struct promise;

  template <typename R>
  Promise<R> ThenInternal(
      Optional<Location> from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<PromiseResult<R>(NonVoidT)> on_resolve,
      OnceCallback<PromiseResult<R>(const RejectValue&)> on_reject =
          OnceCallback<PromiseResult<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<ChainedPromise<R, NonVoidT>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternal(
      Optional<Location> from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<Promise<R>(NonVoidT)> on_resolve,
      OnceCallback<Promise<R>(const RejectValue&)> on_reject =
          OnceCallback<Promise<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<CurringPromise<R, NonVoidT>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternal(Optional<Location> from_here,
                          scoped_refptr<SequencedTaskRunner> task_runner,
                          OnceCallback<R(NonVoidT)> on_resolve,
                          OnceCallback<R(const RejectValue&)> on_reject =
                              OnceCallback<R(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<NoRejectChainedPromise<R, NonVoidT>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternalVoidT(
      Optional<Location> from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<PromiseResult<R>()> on_resolve,
      OnceCallback<PromiseResult<R>(const RejectValue&)> on_reject =
          OnceCallback<PromiseResult<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<ChainedPromise<R, void>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternalVoidT(
      Optional<Location> from_here,
      scoped_refptr<SequencedTaskRunner> task_runner,
      OnceCallback<Promise<R>()> on_resolve,
      OnceCallback<Promise<R>(const RejectValue&)> on_reject =
          OnceCallback<Promise<R>(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<CurringPromise<R, void>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> ThenInternalVoidT(Optional<Location> from_here,
                               scoped_refptr<SequencedTaskRunner> task_runner,
                               OnceCallback<R()> on_resolve,
                               OnceCallback<R(const RejectValue&)> on_reject =
                                   OnceCallback<R(const RejectValue&)>()) {
    return Promise<R>(MakeRefCounted<NoRejectChainedPromise<R, void>>(
        from_here, task_runner, internal_promise_, std::move(on_resolve),
        std::move(on_reject)));
  }

  scoped_refptr<InternalPromise<T>> internal_promise_;
};

struct BASE_EXPORT promise {
 private:
  template <typename Promise>
  static scoped_refptr<PromiseBase> GetPromiseBase(Promise p) {
    return p.internal_promise_;
  }

  template <typename Promise>
  static auto& GetPromiseValueRef(Promise p) {
    return p.internal_promise_->value();
  }

 public:
  /*
   * Accepts one or more promises and returns a
   * Promise<std::tuple<PromiseType> ...>> which is resolved when all promises
   * resolved or rejects with the RejectValue of the first promise to do reject.
   */
  template <typename... Promises>
  static auto All(Promises... promises) {
    using ReturnType =
        std::tuple<typename UnwrapPromise<Promises>::non_void_type...>;
    return Promise<ReturnType>(
        new AllPromise<ReturnType, Promises...>({GetPromiseBase(promises)...}));
  }

  /*
   * Accepts one or more promises and returns a
   * Promise<Variant<distinct non-void promise types>> which is resolved
   * when any input promise is resolved or rejected.
   */
  template <typename... Promises>
  static auto Race(Promises... promises) {
    using ReturnType = typename DedupVariant<
        typename UnwrapPromise<Promises>::non_void_type...>::type;
    return Promise<ReturnType>(new RacePromise<ReturnType, Promises...>(
        {GetPromiseBase(promises)...}));
  }

  /* Returns a rejected promise.*/
  template <typename T>
  static Promise<T> Reject(Value err) {
    return Promise<T>(new InternalPromise<T>(RejectValue(std::move(err))));
  }

  /* Returns a resolved promise.*/
  template <typename T>
  static Promise<T> Resolve(T&& t) {
    return Promise<T>(new InternalPromise<T>(std::forward<T>(t)));
  }

  static Promise<void> Resolve() {
    return Promise<void>(new InternalPromise<void>(Void()));
  }
};

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_H_
