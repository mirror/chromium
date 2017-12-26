// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_H_
#define BASE_PROMISE_PROMISE_H_

#include "base/promise/promise_internal.h"

namespace base {
struct promise;

class SequencedTaskRunner;

template <typename T, typename ArgT>
class CurringPromise;

// base::Promise is inspired by ES6 promises and has more or less the same API.
// Because C++ isn't a dynamic language (std::any not withstanding) there are
// some practical differences:
// 1. base::Value is used for reject reason. While it's possible to templateize
//    the reject type, we'd end up having to store the reject reason in a
//    std::any to cope with chained Thens.
// 2. promise::Race returns a de-duped base::Variant<> of the resolve types.
// 3. promise::All returns a
//    std::tuple<base::Variant<Resolve<T>, Reject<Value>>...>
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
template <typename T>
class Promise {
 public:
  Promise(OnceCallback<void(PromiseResolver<T>)> initalizer) {
    scoped_refptr<InitialPromise<T>> promise(new InitialPromise<T>());
    std::move(initalizer).Run(PromiseResolver<T>(promise));
    internal_promise_ = promise;
  }

  explicit Promise(scoped_refptr<InternalPromise<T>> internal_promise)
      : internal_promise_(internal_promise) {
    internal_promise_->PromiseBase::ExecuteIfPossible();
  }

 public:
  // |on_reject| is executed if the parent promise is rejected. Based on the
  // return value of |on_reject| the promise returned by the Catch will either
  // be resolved, rejected or the result of a curried promise.
  template <typename R>
  Promise<R> Catch(OnceCallback<PromiseResult<R>(const Value&)> on_reject);

  // A task to execute |on_reject| is posted if the parent promise is rejected.
  // Based on the return value of |on_reject| the promise returned by the Catch
  // will either be resolved, rejected or the result of a curried promise.
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   OnceCallback<PromiseResult<R>(const Value&)> on_reject);
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   scoped_refptr<SequencedTaskRunner>& task_runner,
                   OnceCallback<PromiseResult<R>(const Value&)> on_reject);

  // |on_reject| is executed as soon as the parent promise rejected.
  // |on_resolve| returns a Promise<R> the result of which is curried.
  template <typename R>
  Promise<R> Catch(OnceCallback<Promise<R>(const Value&)> on_reject);

  // A task to execute |on_reject| is posted if the parent promise is rejected.
  // |on_resolve| returns a Promise<R> the result of which is curried.
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   OnceCallback<Promise<R>(const Value&)> on_reject);
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   scoped_refptr<SequencedTaskRunner>& task_runner,
                   OnceCallback<Promise<R>(const Value&)> on_reject);

  // |on_reject| is executed as soon as the parent promise rejected. Returns a
  // promise for the return value of |on_reject|.
  template <typename R>
  Promise<R> Catch(OnceCallback<R(const Value&)> on_reject);

  // A task to execute |on_reject| is posted if the parent promise is rejected.
  // Returns a promise for the return value of |on_reject|.
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   OnceCallback<R(const Value&)> on_reject);
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   scoped_refptr<SequencedTaskRunner>& task_runner,
                   OnceCallback<R(const Value&)> on_reject);

  // |on_resolve| or |on_reject| are executed as soon as the parent promise
  // is resolved or rejected. Based on the return value of |on_resolve| the
  // promise returned by the Then will either be resolved, rejected or the
  // result of a curried promise.
  template <typename R>
  Promise<R> Then(OnceCallback<PromiseResult<R>(T)> on_resolve,
                  OnceCallback<PromiseResult<R>(const Value&)> on_reject =
                      OnceCallback<PromiseResult<R>(const Value&)>());

  // A task to execute |on_resolve| or |on_reject| is posted as soon as the
  // parent promise is resolved or rejected.  The |on_resolve| function can
  // choose to resolve by returning Resolve<R> (or R if it's not ambiguous) or
  // reject by returning Reject<Value> (or Value if it's not ambiguous).
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  OnceCallback<PromiseResult<R>(T)> on_resolve,
                  OnceCallback<PromiseResult<R>(const Value&)> on_reject =
                      OnceCallback<PromiseResult<R>(const Value&)>());
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  scoped_refptr<SequencedTaskRunner>& task_runner,
                  OnceCallback<PromiseResult<R>(T)> on_resolve,
                  OnceCallback<PromiseResult<R>(const Value&)> on_reject =
                      OnceCallback<PromiseResult<R>(const Value&)>());

  // |on_resolve| or |on_reject| are executed as soon as the parent promise is
  // resolved or rejected. |on_resolve| returns a Promise<R> the result of which
  // is curried.
  template <typename R>
  Promise<R> Then(OnceCallback<Promise<R>(T)> on_resolve,
                  OnceCallback<Promise<R>(const Value&)> on_reject =
                      OnceCallback<Promise<R>(const Value&)>());

  // A task to execute |on_resolve| or |on_reject| is posted as soon as the
  // parent promise is resolved or rejected. |on_resolve| returns a Promise<R>
  // the result of which is curried.
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  OnceCallback<Promise<R>(T)> on_resolve,
                  OnceCallback<Promise<R>(const Value&)> on_reject =
                      OnceCallback<Promise<R>(const Value&)>());

  template <typename R>
  Promise<R> Then(const Location& from_here,
                  scoped_refptr<SequencedTaskRunner>& task_runner,
                  OnceCallback<Promise<R>(T)> on_resolve,
                  OnceCallback<Promise<R>(const Value&)> on_reject =
                      OnceCallback<Promise<R>(const Value&)>());

  // |on_resolve| or |on_reject| are executed as soon as the parent promise
  // is resolved or rejected. |on_resolve| cant reject.
  template <typename R>
  Promise<R> Then(OnceCallback<R(T)> on_resolve,
                  OnceCallback<R(const Value&)> on_reject =
                      OnceCallback<R(const Value&)>());

  // A task to execute |on_resolve| or |on_reject| is posted as soon as the
  // parent promise is resolved or rejected. |on_resolve| cant reject.
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  OnceCallback<R(T)> on_resolve,
                  OnceCallback<R(const Value&)> on_reject =
                      OnceCallback<R(const Value&)>());
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  scoped_refptr<SequencedTaskRunner>& task_runner,
                  OnceCallback<R(T)> on_resolve,
                  OnceCallback<R(const Value&)> on_reject =
                      OnceCallback<R(const Value&)>());

  // |finally_callback| is called after the parent promise is resolved or
  // rejected and all Thens have executed.
  void Finally(OnceClosure finally_callback);

  // A task to execute |finally_callback| is posted after the parent promise is
  // resolved or rejected and all Thens have executed.
  void Finally(const Location& from_here, OnceClosure finally_callback);
  void Finally(const Location& from_here,
               OnceClosure finally_callback,
               scoped_refptr<SequencedTaskRunner>& task_runner);

 private:
  template <typename TT, typename ArgT>
  friend class CurringPromise;

  template <typename TT>
  friend struct PromiseResult;

  friend struct promise;

  scoped_refptr<InternalPromise<T>> internal_promise_;
};

template <>
class Promise<void> {
 public:
  Promise(OnceCallback<void(PromiseResolver<void>)> initalizer) {
    scoped_refptr<InitialPromise<void>> promise(new InitialPromise<void>());
    std::move(initalizer).Run(PromiseResolver<void>(promise));
    internal_promise_ = promise;
  }

  explicit Promise(scoped_refptr<InternalPromise<void>> internal_promise)
      : internal_promise_(internal_promise) {
    internal_promise_->PromiseBase::ExecuteIfPossible();
  }

 public:
  // |on_reject| is executed if the parent promise is rejected. Based on the
  // return value of |on_reject| the promise returned by the Catch will either
  // be resolved, rejected or the result of a curried promise.
  template <typename R>
  Promise<R> Catch(OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
    return Promise<R>(new ChainedPromise<R, void>(
        nullopt, {internal_promise_}, OnceCallback<PromiseResult<R>()>(),
        std::move(on_reject)));
  }

  // A task to execute |on_reject| is posted if the parent promise is rejected.
  // Based on the return value of |on_reject| the promise returned by the Catch
  // will either be resolved, rejected or the result of a curried promise.
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
    return Promise<R>(new ChainedPromise<R, void>(
        from_here, {internal_promise_}, OnceCallback<PromiseResult<R>()>(),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   scoped_refptr<SequencedTaskRunner>& task_runner,
                   OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
    return Promise<R>(new ChainedPromise<R, void>(
        from_here, {internal_promise_}, OnceCallback<PromiseResult<R>()>(),
        std::move(on_reject)));
  }

  // |on_reject| is executed as soon as the parent promise rejected.
  // |on_resolve| returns a Promise<R> the result of which is curried.
  template <typename R>
  Promise<R> Catch(OnceCallback<Promise<R>(const Value&)> on_reject) {
    return Promise<R>(new CurringPromise<R, void>(nullopt, {internal_promise_},
                                                  OnceCallback<Promise<R>()>(),
                                                  std::move(on_reject)));
  }

  // A task to execute |on_reject| is posted if the parent promise is rejected.
  // |on_resolve| returns a Promise<R> the result of which is curried.
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   OnceCallback<Promise<R>(const Value&)> on_reject) {
    return Promise<R>(new CurringPromise<R, void>(
        from_here, {internal_promise_}, OnceCallback<Promise<R>()>(),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   scoped_refptr<SequencedTaskRunner>& task_runner,
                   OnceCallback<Promise<R>(const Value&)> on_reject) {
    return Promise<R>(new CurringPromise<R, void>(
        from_here, {internal_promise_}, OnceCallback<Promise<R>()>(),
        std::move(on_reject)));
  }

  // |on_reject| is executed as soon as the parent promise rejected. Returns a
  // promise for the return value of |on_reject|.
  template <typename R>
  Promise<R> Catch(OnceCallback<R(const Value&)> on_reject) {
    return Promise<R>(new NoRejectChainedPromise<R, void>(
        nullopt, {internal_promise_}, OnceCallback<R()>(),
        std::move(on_reject)));
  }

  // A task to execute |on_reject| is posted if the parent promise is rejected.
  // Returns a promise for the return value of |on_reject|.
  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   OnceCallback<R(const Value&)> on_reject) {
    return Promise<R>(new NoRejectChainedPromise<R, void>(
        from_here, {internal_promise_}, OnceCallback<R()>(),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> Catch(const Location& from_here,
                   scoped_refptr<SequencedTaskRunner>& task_runner,
                   OnceCallback<R(const Value&)> on_reject) {
    return Promise<R>(new NoRejectChainedPromise<R, void>(
        from_here, {internal_promise_}, OnceCallback<R()>(),
        std::move(on_reject)));
  }

  // |on_resolve| or |on_reject| are executed as soon as the parent promise
  // is resolved or rejected.  The |on_resolve| function can choose to resolve
  // by returning Resolve<R> (or R if it's not ambiguous) or reject by
  // returning Reject<Value> (or Value if it's not ambiguous).
  template <typename R>
  Promise<R> Then(OnceCallback<PromiseResult<R>()> on_resolve,
                  OnceCallback<PromiseResult<R>(const Value&)> on_reject =
                      OnceCallback<PromiseResult<R>(const Value&)>()) {
    return Promise<R>(new ChainedPromise<R, void>(nullopt, {internal_promise_},
                                                  std::move(on_resolve),
                                                  std::move(on_reject)));
  }

  // A task to execute |on_resolve| or |on_reject| is posted as soon as the
  // parent promise is resolved or rejected.  The |on_resolve| function can
  // choose to resolve by returning Resolve<R> (or R if it's not ambiguous) or
  // reject by returning Reject<Value> (or Value if it's not ambiguous).
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  OnceCallback<PromiseResult<R>()> on_resolve,
                  OnceCallback<PromiseResult<R>(const Value&)> on_reject =
                      OnceCallback<PromiseResult<R>(const Value&)>()) {
    return Promise<R>(new ChainedPromise<R, void>(
        from_here, {internal_promise_}, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> Then(const Location& from_here,
                  scoped_refptr<SequencedTaskRunner>& task_runner,
                  OnceCallback<PromiseResult<R>()> on_resolve,
                  OnceCallback<PromiseResult<R>(const Value&)> on_reject =
                      OnceCallback<PromiseResult<R>(const Value&)>()) {
    return Promise<R>(new ChainedPromise<R, void>(
        from_here, {internal_promise_}, std::move(on_resolve),
        std::move(on_reject)));
  }

  // |on_resolve| or |on_reject| are executed as soon as the parent promise is
  // resolved or rejected. |on_resolve| returns a Promise<R>
  // the result of which is curried.
  template <typename R>
  Promise<R> Then(OnceCallback<Promise<R>()> on_resolve,
                  OnceCallback<Promise<R>(const Value&)> on_reject =
                      OnceCallback<Promise<R>(const Value&)>()) {
    return Promise<R>(new CurringPromise<R, void>(nullopt, {internal_promise_},
                                                  std::move(on_resolve),
                                                  std::move(on_reject)));
  }

  // A task to execute |on_resolve| or |on_reject| is posted as soon as the
  // parent promise is resolved or rejected. |on_resolve| returns a Promise<R>
  // the result of which is curried.
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  OnceCallback<Promise<R>()> on_resolve,
                  OnceCallback<Promise<R>(const Value&)> on_reject =
                      OnceCallback<Promise<R>(const Value&)>()) {
    return Promise<R>(new CurringPromise<R, void>(
        from_here, {internal_promise_}, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> Then(const Location& from_here,
                  scoped_refptr<SequencedTaskRunner>& task_runner,
                  OnceCallback<Promise<R>()> on_resolve,
                  OnceCallback<Promise<R>(const Value&)> on_reject =
                      OnceCallback<Promise<R>(const Value&)>()) {
    return Promise<R>(new CurringPromise<R, void>(
        from_here, {internal_promise_}, std::move(on_resolve),
        std::move(on_reject)));
  }

  // |on_resolve| or |on_reject| are executed as soon as the parent promise
  // is resolved or rejected. |on_resolve| cant reject.
  template <typename R>
  Promise<R> Then(OnceCallback<R()> on_resolve,
                  OnceCallback<R(const Value&)> on_reject =
                      OnceCallback<R(const Value&)>()) {
    return Promise<R>(new NoRejectChainedPromise<R, void>(
        nullopt, {internal_promise_}, std::move(on_resolve),
        std::move(on_reject)));
  }

  // A task to execute |on_resolve| or |on_reject| is posted as soon as the
  // parent promise is resolved or rejected. |on_resolve| cant reject.
  template <typename R>
  Promise<R> Then(const Location& from_here,
                  OnceCallback<R()> on_resolve,
                  OnceCallback<R(const Value&)> on_reject =
                      OnceCallback<R(const Value&)>()) {
    return Promise<R>(new NoRejectChainedPromise<R, void>(
        from_here, {internal_promise_}, std::move(on_resolve),
        std::move(on_reject)));
  }

  template <typename R>
  Promise<R> Then(const Location& from_here,
                  scoped_refptr<SequencedTaskRunner>& task_runner,
                  OnceCallback<R()> on_resolve,
                  OnceCallback<R(const Value&)> on_reject =
                      OnceCallback<R(const Value&)>()) {
    return Promise<R>(new NoRejectChainedPromise<R, void>(
        from_here, {internal_promise_}, std::move(on_resolve),
        std::move(on_reject)));
  }

  // A task to execute |on_resolve| or |on_reject| is posted as soon as the
  // parent promise is resolved or rejected.  |on_resolve| cant reject.
  void Finally(OnceClosure finally_callback) {
    new FinallyPromise(nullopt, {internal_promise_},
                       std::move(finally_callback));
  }

  // A task to execute |finally_callback| is posted after the parent promise is
  // resolved or rejected and all Thens have executed.
  void Finally(const Location& from_here, OnceClosure finally_callback) {
    new FinallyPromise(from_here, {internal_promise_},
                       std::move(finally_callback));
  }

  void Finally(const Location& from_here,
               OnceClosure finally_callback,
               scoped_refptr<SequencedTaskRunner>& task_runner) {
    new FinallyPromise(from_here, {internal_promise_},
                       std::move(finally_callback));
  }

 private:
  template <typename T, typename ArgT>
  friend class CurringPromise;

  template <typename TT>
  friend struct PromiseResult;

  friend struct promise;

  scoped_refptr<InternalPromise<void>> internal_promise_;
};

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(
    OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
  return Promise<R>(new ChainedPromise<R, T>(
      nullopt, {internal_promise_}, OnceCallback<PromiseResult<R>(T)>(),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(
    const Location& from_here,
    OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
  return Promise<R>(new ChainedPromise<R, T>(
      from_here, {internal_promise_}, OnceCallback<PromiseResult<R>(T)>(),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(
    const Location& from_here,
    scoped_refptr<SequencedTaskRunner>& task_runner,
    OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
  return Promise<R>(new ChainedPromise<R, T>(
      from_here, {internal_promise_}, OnceCallback<PromiseResult<R>(T)>(),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(OnceCallback<R(const Value&)> on_reject) {
  return Promise<R>(new NoRejectChainedPromise<R, T>(
      nullopt, {internal_promise_}, OnceCallback<R(T)>(),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(const Location& from_here,
                             OnceCallback<R(const Value&)> on_reject) {
  return Promise<R>(new NoRejectChainedPromise<R, T>(
      from_here, {internal_promise_}, OnceCallback<R(T)>(),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(const Location& from_here,
                             scoped_refptr<SequencedTaskRunner>& task_runner,
                             OnceCallback<R(const Value&)> on_reject) {
  return Promise<R>(new NoRejectChainedPromise<R, T>(
      from_here, {internal_promise_}, OnceCallback<R(T)>(),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(OnceCallback<Promise<R>(const Value&)> on_reject) {
  return Promise<R>(new CurringPromise<R, T>(nullopt, {internal_promise_},
                                             OnceCallback<Promise<R>()>(),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(const Location& from_here,
                             OnceCallback<Promise<R>(const Value&)> on_reject) {
  return Promise<R>(new CurringPromise<R, T>(from_here, {internal_promise_},
                                             OnceCallback<Promise<R>()>(),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Catch(const Location& from_here,
                             scoped_refptr<SequencedTaskRunner>& task_runner,
                             OnceCallback<Promise<R>(const Value&)> on_reject) {
  return Promise<R>(new CurringPromise<R, T>(from_here, {internal_promise_},
                                             OnceCallback<Promise<R>()>(),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(
    OnceCallback<PromiseResult<R>(T)> on_resolve,
    OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
  return Promise<R>(new ChainedPromise<R, T>(nullopt, {internal_promise_},
                                             std::move(on_resolve),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(
    const Location& from_here,
    OnceCallback<PromiseResult<R>(T)> on_resolve,
    OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
  return Promise<R>(new ChainedPromise<R, T>(from_here, {internal_promise_},
                                             std::move(on_resolve),
                                             std::move(on_reject)));
}
template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(
    const Location& from_here,
    scoped_refptr<SequencedTaskRunner>& task_runner,
    OnceCallback<PromiseResult<R>(T)> on_resolve,
    OnceCallback<PromiseResult<R>(const Value&)> on_reject) {
  return Promise<R>(new ChainedPromise<R, T>(from_here, {internal_promise_},
                                             std::move(on_resolve),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(OnceCallback<Promise<R>(T)> on_resolve,
                            OnceCallback<Promise<R>(const Value&)> on_reject) {
  return Promise<R>(new CurringPromise<R, T>(nullopt, {internal_promise_},
                                             std::move(on_resolve),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(const Location& from_here,
                            OnceCallback<Promise<R>(T)> on_resolve,
                            OnceCallback<Promise<R>(const Value&)> on_reject) {
  return Promise<R>(new CurringPromise<R, T>(from_here, {internal_promise_},
                                             std::move(on_resolve),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(const Location& from_here,
                            scoped_refptr<SequencedTaskRunner>& task_runner,
                            OnceCallback<Promise<R>(T)> on_resolve,
                            OnceCallback<Promise<R>(const Value&)> on_reject) {
  return Promise<R>(new CurringPromise<R, T>(from_here, {internal_promise_},
                                             std::move(on_resolve),
                                             std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(OnceCallback<R(T)> on_resolve,
                            OnceCallback<R(const Value&)> on_reject) {
  return Promise<R>(new NoRejectChainedPromise<R, T>(
      nullopt, {internal_promise_}, std::move(on_resolve),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(const Location& from_here,
                            OnceCallback<R(T)> on_resolve,
                            OnceCallback<R(const Value&)> on_reject) {
  return Promise<R>(new NoRejectChainedPromise<R, T>(
      from_here, {internal_promise_}, std::move(on_resolve),
      std::move(on_reject)));
}

template <typename T>
template <typename R>
Promise<R> Promise<T>::Then(const Location& from_here,
                            scoped_refptr<SequencedTaskRunner>& task_runner,
                            OnceCallback<R(T)> on_resolve,
                            OnceCallback<R(const Value&)> on_reject) {
  return Promise<R>(new NoRejectChainedPromise<R, T>(
      from_here, {internal_promise_}, std::move(on_resolve),
      std::move(on_reject)));
}

template <typename T>
void Promise<T>::Finally(OnceClosure finally_callback) {
  new FinallyPromise(nullopt, {internal_promise_}, std::move(finally_callback));
}

template <typename T>
void Promise<T>::Finally(const Location& from_here,
                         OnceClosure finally_callback) {
  new FinallyPromise(from_here, {internal_promise_},
                     std::move(finally_callback));
}

struct promise {
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
   * Promise<std::tuple<Variant<Resolve<PromiseType>, Reject<Value>>& ...>>
   * which is resolved when all promises are settled (i.e. they are resolved or
   * rejected).
   */
  template <typename... Promises>
  static auto All(Promises... promises) {
    using ReturnType = std::tuple<
        Variant<base::Resolve<typename UnwrapPromise<Promises>::type>,
                base::Reject<Value>>...>;
    return Promise<ReturnType>(new AllPromise<ReturnType, Promises...>(
        nullopt, {GetPromiseBase(promises)...}));
  }

  /*
   * Accepts one or more promises and returns a
   * Promise<Variant<distinct non-void promise types>> which is resolved
   * when any input promise is resolved or rejected.
   */
  template <typename... Promises>
  static auto Race(Promises... promises) {
    using ReturnType = typename DedupAndRemoveVoid<
        typename UnwrapPromise<Promises>::type...>::type;
    return Promise<ReturnType>(new RacePromise<ReturnType, Promises...>(
        nullopt, {GetPromiseBase(promises)...}));
  }

  /* Returns a rejected promise.*/
  template <typename T>
  static Promise<T> Reject(Value err) {
    return Promise<T>(
        new InternalPromise<T>(base::Reject<Value>(std::move(err))));
  }

  /* Returns a resolved promise.*/
  template <typename T>
  static Promise<T> Resolve(T&& t) {
    return Promise<T>(
        new InternalPromise<T>(base::Resolve<T>(std::forward<T>(t))));
  }
};

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_H_
