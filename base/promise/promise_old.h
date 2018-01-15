// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

#include <atomic>
#include <tuple>
#include <utility>

namespace base {

struct NotReached {
  NotReached() = delete;
  NotReached(const NotReached&) = delete;
  NotReached(NotReached&&) = delete;
  NotReached& operator=(const NotReached&) = delete;
  NotReached& operator=(NotReached&&) = delete;
};

template <typename ResolveValue, typename RejectValue = void>
class Promise;

template <typename ResolveValue, typename RejectValue = void>
class Future;

namespace internal {

template <typename ResolveValue, typename RejectValue>
struct WrapFutureImpl {
  using Type = Future<ResolveValue, RejectValue>;
};

template <typename ResolveValue,
          typename InnerRejectValue,
          typename RejectValue>
struct WrapFutureImpl<Future<ResolveValue, InnerRejectValue>, RejectValue> {
  static_assert(std::is_same<InnerRejectValue, RejectValue>::value, "");
  using Type = Future<ResolveValue, RejectValue>;
};

template <typename ResolveValue, typename RejectValue = void>
using WrapFuture = WrapFutureImpl<ResolveValue, RejectValue>;

template <typename ResolveValue, typename RejectValue>
struct PromiseValue {
  using Resolved = std::conditional_t<std::is_void<ResolveValue>::value,
                                      bool,
                                      Optional<ResolveValue>>;
  using Rejected = std::conditional_t<std::is_void<RejectValue>::value,
                                      bool,
                                      Optional<RejectValue>>;

  Resolved resolved_;
  Rejected rejected_;
};

template <typename X, typename Y>
Optional<Y> Transform(OnceCallback<Y(X)> cb, Optional<X> x) {
  return x ? std::move(cb).Run(*x) : nullopt;
}

template <typename Y>
Optional<Y> Transform(OnceCallback<Y()> cb, bool x) {
  return x ? std::move(cb).Run() : nullopt;
}

template <typename X>
bool Transform(OnceCallback<void(X)> cb, Optional<X> x) {
  if (x)
    std::move(cb).Run(*x);
  return x.has_value();
}

inline bool Transform(OnceClosure cb, bool x) {
  if (x)
    std::move(cb).Run();
  return x;
}

template <typename Value>
class PromiseStorage {
 public:
  PromiseStorage() {}
  ~PromiseStorage() = default;

  void Resolve(const Location& from_here, Value value) {
    DCHECK(!value_);
    from_here_ = from_here;
    value_.emplace(std::move(value));
    Proceed();
  }

  void Attach(scoped_refptr<TaskRunner> task_runner,
              OnceCallback<void(Value*)> handler) {
    DCHECK(!task_runner_);
    DCHECK(!handler_);
    task_runner_ = std::move(task_runner);
    handler_ = std::move(handler);
    Proceed();
  }

  void OnDefaulted() {
    DCHECK(!value_);
    Proceed();
  }

  void OnAbandoned() {
    DCHECK(!handler_);
    Proceed();
  }

 private:
  void Proceed() {
    if (--progress_)
      return;

    std::unique_ptr<PromiseStorage> deleter(this);
    if (!resolve_handler_) {
      // OnAbandoned() case. The Future object is destroyed without attaching to
      // a handler.
      return;
    }

    DCHECK(task_runner_);
    if (value_) {
      task_runner_->PostTask(from_here_,
                             BindOnce(std::move(handler_), std::move(*value)));
      return;
    }
  }

  std::atomic<int> progress_ = 2;

  scoped_refptr<TaskRunner> task_runner_;
  OnceCallback<void(Value)> handler_;
  Location from_here_;
  Optional<Value> value_;
};

}  // namespace internal

// Creates a pair of Future and Promise.
template <typename ResolveValue, typename RejectValue = void>
std::tuple<Future<ResolveValue, RejectValue>,
           Promise<ResolveValue, RejectValue>>
Contract() {
  using Value = internal::PromiseValue<ResolveValue, RejectValue>;
  using Storage = internal::PromiseStorage<Value>;
  Storage* storage = new Storage;
  return std::make_tuple(Future<ResolveValue, RejectValue>(storage),
                         Promise<ResolveValue, RejectValue>(storage));
}

// Represents a value that will be available later.
template <typename ResolveValue, typename RejectValue>
class Future {
 private:
  using Value = internal::PromiseValue<ResolveValue, RejectValue>;
  using Storage = internal::PromiseStorage<Value>;

 public:
  // Future is a move-only type.
  Future() = default;
  Future(Future&&) = default;
  Future& operator=(Future&&) = default;
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;
  ~Future() = default;

  // template <typename R>
  // WrapFuture<R, RejectValue> Chain(const Location& from_here,
  //                                  OnceCallback<R(ResolveValue)> handler) {
  //   Future<R, RejectValue> future;
  //   Promise<R, RejectValue> promise;
  //   std::tie(future, promise) = Contract<R>();

  //   auto relay = [](const Location& from_here, OnceCallback<R(Value)>
  //   handler,
  //                   Promise<R> chained, Value value) {
  //     std::move(chained).Resolve(from_here,
  //                                std::move(handler).Run(std::move(value)));
  //   };

  //   std::move(*this).Then(
  //       std::move(task_runner),
  //       BindOnce(relay, from_here, std::move(handler), std::move(promise)));
  //   return future;
  // }

  // template <typename X, typename Y>
  // WrapFuture<R, RejectValue> Chain(const Location& from_here,
  //                                  OnceCallback<X(ResolveValue)> handler,
  //                                  OnceCallback<Y(RejectValue)>) {
  //   Future<R, RejectValue> future;
  //   Promise<R, RejectValue> promise;
  //   std::tie(future, promise) = Contract<R>();

  //   auto relay = [](const Location& from_here, OnceCallback<R(Value)>
  //   handler,
  //                   Promise<R> chained, Value value) {
  //     std::move(chained).Resolve(from_here,
  //                                std::move(handler).Run(std::move(value)));
  //   };

  //   std::move(*this).Then(
  //       std::move(task_runner),
  //       BindOnce(relay, from_here, std::move(handler), std::move(promise)));
  //   return future;
  // }

  // template <typename R>
  // Future<R> Chain(scoped_refptr<TaskRunner> task_runner,
  //                 const Location& from_here,
  //                 OnceCallback<R(ResolveValue)> handler) && {
  //   Future<R> future;
  //   Promise<R> promise;
  //   std::tie(future, promise) = Contract<R>();

  //   auto relay = [](const Location& from_here, OnceCallback<R(Value)>
  //   handler,
  //                   Promise<R> chained, Value value) {
  //     std::move(chained).Resolve(from_here,
  //                                std::move(handler).Run(std::move(value)));
  //   };

  //   std::move(*this).Then(
  //       std::move(task_runner),
  //       BindOnce(relay, from_here, std::move(handler), std::move(promise)));
  //   return future;
  // }

  // template <typename R>
  // Future<R> Chain(const Location& from_here,
  //                 OnceCallback<R(Value)> handler) && {
  //   Future<R> future;
  //   Promise<R> promise;
  //   std::tie(future, promise) = Contract<R>();

  //   auto relay = [](const Location& from_here, OnceCallback<R(Value)>
  //   handler,
  //                   Promise<R> chained, Value value) {
  //     std::move(chained).Resolve(from_here,
  //                                std::move(handler).Run(std::move(value)));
  //   };

  //   std::move(*this).Then(
  //       BindOnce(relay, from_here, std::move(handler), std::move(promise)));
  //   return future;
  // }

  // void Then(OnceCallback<void(Value)> handler) && {
  //   std::move(*this).Then(SequencedTaskRunnerHandle::Get(),
  //   std::move(handler));
  // }

  // void Then(scoped_refptr<TaskRunner> task_runner,
  //           OnceCallback<void(Value)> handler) && {
  //   DCHECK(storage_);
  //   internal::PromiseStorage<Value>* storage = storage_.release();
  //   storage->Attach(std::move(task_runner), std::move(handler));
  // }

  // static Future CreateResolved(ResolveValue);
  // static Future CreateRejected(RejectValue);

  template <typename R>
  Future<R, RejectValue> Then(const Location& from_here,
                              OnceCallback<R(ResolveValue)> cb) {
    Future<R, RejectValue> future;
    Promise<R, RejectValue> promise;
    std::tie(future, promise) = Contract<R, RejectValue>();

    auto relay = [](const Location& from_here, OnceCallback<R(ResolveValue)> cb,
                    Promise<R, RejectValue> promise, Value value) {
      PromiseValue<R, RejectedValue> res;
      res.resolved = Transform(std::move(cb), std::move(value.resolved));
      res.rejected = std::move(value.rejected);
      promise.Settle(from_here, std::move(res));
    };
    std::move(*this).ThenImpl(
        std::move(task_runner),
        BindOnce(relay, from_here, std::move(cb), std::move(promise)));
    return future;
  }

  template <typename R>
  Future<R, RejectValue> Then(
      const Location& from_here,
      OnceCallback<Future<R, RejectValue>(ResolveValue)> cb) {
    Future<R, RejectValue> future;
    Promise<R, RejectValue> promise;
    std::tie(future, promise) = Contract<R, RejectValue>();

    auto relay = [](const Location& from_here,
                    OnceCallback<Future<R, RejectedValue>()> cb,
                    Promise<R, RejectValue> promise, Value value) {
      if (value.rejected) {
        PromiseValue<R, RejectValue> res;
        res.rejected = std::move(value.rejected);
        promise.Settle(from_here, std::move(res));
        return;
      }

      DCHECK(value.resolved);
      promise.Settle(frome_here,
                     *Transform(std::move(cb), std::move(value.resolved)));
    };
    std::move(*this).ThenImpl(
        std::move(task_runner),
        BindOnce(relay, from_here, std::move(cb), std::move(promise)));
    return future;
  }

  // WrapFuture<R, RejectValue> Catch(const Location& from_here,
  //                                  OnceCallback<R(RejectValue)> cb) {
  //   using Wrapped = WrapFuture<R, RejectValue>;
  //   using ResolveValue = typename Wrapped::ResolveValue;

  //   Future<ResolveValue, RejectValue> future;
  //   Promise<ResolveValue, RejectValue> promise;
  //   std::tie(future, promise) = Contract<ResolveValue, RejectValue>();

  //   using RelayFuncType = void (*)(
  //       const Location&, OnceCallback<R(ResolveValue)>, Wrapped, Value);
  //   RelayFuncType relay = &Relay;
  //   std::move(*this).ThenImpl(
  //       std::move(task_runner),
  //       BindOnce(&relay, from_here, std::move(cb), std::move(promise)));
  //   return future;
  // }

  template <typename R>
  Future<R, NotReached> Catch(const Location& from_here,
                              OnceCallback<R(RejectValue)> cb) {
    Future<R, NotReached> future;
    Promise<R, NotReached> promise;
    std::tie(future, promise) = Contract<R, NotReached>();

    auto relay = [](const Location& from_here, OnceCallback<R(ResolveValue)> cb,
                    Promise<R, RejectValue> promise, Value value) {
      PromiseValue<R, RejectedValue> res;
      res.resolved = Transform(std::move(cb), std::move(value.resolved));
      res.rejected = std::move(value.rejected);
      promise.Settle(from_here, std::move(res));
    };
    std::move(*this).ThenImpl(
        std::move(task_runner),
        BindOnce(relay, from_here, std::move(cb), std::move(promise)));
    return future;
  }

  template <typename R>
  Future<R, RejectValue> Then(
      const Location& from_here,
      OnceCallback<Future<R, RejectValue>(ResolveValue)> cb) {
    using Wrapped = WrapFuture<R, RejectValue>;
    using ResolveValue = typename Wrapped::ResolveValue;

    Future<R, RejectValue> future;
    Promise<R, RejectValue> promise;
    std::tie(future, promise) = Contract<R, RejectValue>();

    auto relay = [](const Location& from_here,
                    OnceCallback<Future<R, RejectedValue>()> cb,
                    Promise<R, RejectValue> promise, Value value) {
      if (value.rejected) {
        PromiseValue<R, RejectValue> res;
        res.rejected = std::move(value.rejected);
        promise.Settle(from_here, std::move(res));
        return;
      }

      DCHECK(value.resolved);
      promise.Settle(frome_here,
                     *Transform(std::move(cb), std::move(value.resolved)));
    };
    std::move(*this).ThenImpl(
        std::move(task_runner),
        BindOnce(relay, from_here, std::move(cb), std::move(promise)));
    return future;
  }

 private:
  template <typename R>
  static void Relay(const Location& from_here,
                    OnceCallback<R(ResolveValue)> cb,
                    Promise<R, RejectValue> promise,
                    Value v) {
    if (v.rejected) {
      DCHECK(!v.resolved);
      std::move(promise).Reject(from_here, std::move(v.rejected));
      return;
    }

    DCHECK(!v.rejected);
    std::move(promise).Resolve(from_here,
                               std::move(cb).Run(std::move(v.resolved)));
  }

  template <typename R>
  static void Relay(const Location& from_here,
                    OnceCallback<Future<R>(ResolveValue)> cb,
                    Promise<R, RejectValue> promise,
                    Value v) {
    if (v.rejected) {
      DCHECK(!v.resolved);
      std::move(promise).Reject(from_here, std::move(v.rejected));
      return;
    }

    DCHECK(!v.rejected);
    std::move(promise).Apply(from_here,
                             std::move(cb).Run(std::move(v.resolved)));
  }

  void ThenImpl(scoped_refptr<TaskRunner> task_runner,
                OnceCallback<void(Value)> handler) && {
    DCHECK(storage_);
    Storage* storage = storage_.release();
    storage->Attach(std::move(task_runner), std::move(handler));
  }

  void ThenImpl() {}

  friend std::tuple<Future<ResolveValue, RejectValue>,
                    Promise<ResolveValue, RejectValue>>
  Contract<ResolveValue, RejectValue>();

  struct AbandonOnScopeOut {
    void operator()(Storage* storage) const { storage->OnAbandoned(); }
  };

  explicit Future(Storage* storage) : storage_(storage) {}

  std::unique_ptr<Storage, AbandonOnScopeOut> storage_;
};

template <typename ResolveValue, typename RejectValue>
class Promise {
 private:
  using Value = internal::PromiseValue<ResolveValue, RejectValue>;
  using Storage = internal::PromiseStorage<Value>;

 public:
  // Promise is a move-only type.
  Promise() = default;
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;
  ~Promise() = default;

  // Resolves void-valued promise.
  void Resolve(const Location& from_here) && {
    static_assert(std::is_void<ResolveValue>::value,
                  "This Promise needs a value to be resolved with.");
    DCHECK(storage_);
    Value v;
    v.resolved = true;

    Storage* storage = storage_.release();
    storage->Resolve(from_here, std::move(v));
  }

  void Resolve(const Location& from_here, ResolveValue value) && {
    static_assert(!std::is_void<ResolveValue>::value,
                  "This Promise can't take a value.");
    DCHECK(storage_);
    Value v;
    v.resolved.emplace(std::move(value));

    Storage* storage = storage_.release();
    storage->Resolve(from_here, std::move(v));
  }

  void Reject(const Location& from_here) && {
    static_assert(std::is_void<ResolveValue>::value,
                  "This Promise needs a value to be rejected with.");
    DCHECK(storage_);
    Value v;
    v.rejected = true;

    Storage* storage = storage_.release();
    storage->Reject(from_here, std::move(v));
  }

  void Reject(const Location& from_here, RejectValue value) && {
    static_assert(!std::is_void<ResolveValue>::value,
                  "This Promise can't take a value on its rejection.");
    DCHECK(storage_);
    Value v;
    v.rejected.emplace(std::move(value));

    Storage* storage = storage_.release();
    storage->Reject(from_here, std::move(v));
  }

  void Apply(const Location& from_here,
             Future<ResolveValue, RejectValue> future) && {
    std::move(future).ThenImpl(nullptr, BindOnce(&Promise::ResolveOrReject,
                                                 std::move(*this), frome_here));
  }

 private:
  void Settle(const Location& from_here, Value value) {}
  void Settle(const Location& from_here,
              Future<ResolveValue, RejectValue> future) {}

  void RejectInternal(const Locatino& from_here, bool*) {
    std::move(*this).Reject(from_here);
  }

  void RejectInternal(const Locatino& from_here, Optional<RejectValue>* v) {
    std::move(*this).Reject(from_here, std::move(*v));
  }

  void ResolveInternal(const Locatino& from_here, bool*) {
    std::move(*this).Resolve(from_here);
  }

  void ResolveInternal(const Locatino& from_here, Optional<ResolveValue>* v) {
    std::move(*this).Resolve(from_here, std::move(*v));
  }

  static void ResolveOrReject(Promise promise,
                              const Location& from_here,
                              Value v) {
    if (v.rejected) {
      DCHECK(!v.resolved);
      std::move(promise).RejectInternal(from_here, &v.rejected);
      return;
    }

    DCHECK(v.resolved);
    std::move(promise).ResolveInternal(from_here, &v.resolved);
  }

  friend std::tuple<Future<ResolveValue, RejectValue>,
                    Promise<ResolveValueRejectValue>>
  Contract<ResolveValue, RejectValue>();

  struct DefaultOnScopeOut {
    void operator()(Storage* storage) const { storage->OnDefaulted(); }
  };

  explicit Promise(Storage* storage) : storage_(storage) {}

  std::unique_ptr<Storage, DefaultOnScopeOut> storage_;
};

// template <typename... ResolveValues, typename RejectValue>
// Future<std::tuple<ResolveValues...>, RejectValue> Combine(
//     Future<ResolveValues, RejectValue>... futures) {}

// template <typename ResolveValue, typename RejectValue>
// Future<std::vector<ResolveValue>, RejectValue> All(
//     std::vector<Future<ResolveValue, RejectValue>> xs) {}

// template <typename ResolveValue, typename RejectValue>
// Future<ResolveValue, RejectValue> Race(
//     std::vector<Future<ResolveValue, RejectValue>> xs) {}

// template <>
// void Then(Future<ResolveValue, RejectValue>,
//           OnceCallback<void(ResolveValue)>,
//           OnceCallback<void(RejectValue)>);

// template <>
// void Then(Future<ResolveValue, RejectValue>,
// OnceCallback<void(ResolveValue)>);

// template <>
// Future<R, RejectValue> void Chain(Future<ResolveValue, RejectValue>,
//                                   const Location& from_here,
//                                   OnceCallback<X(ResolveValue)>);

// template <>
// Future<R, > void Chain(Future<ResolveValue, RejectValue>,
//                        const Location& from_here,
//                        OnceCallback<X(ResolveValue)>,
//                        OnceCallback<Y(RejectValue)>);

// template <typename ResolveValue>
// Future<ResolveValue, NotReached> Resolve(const Location& from_here,
//                                          ResolveValue value) {
//   Future<ResolveValue, NotReached> future;
//   Promise<ResolveValue, NotReached> promise;
//   std::tie(future, promise) = Contract<ResolveValue, NotReached>();
//   promise.Resolve(from_here, std::move(value));
//   return future;
// }

// template <typename RejectValue>
// Future<NotReached, RejectValue> Reject(const Location& from_here,
// RejectValue) {
//   Future<NotReached, RejectValue> future;
//   Promise<NotReached, RejectValue> promise;
//   std::tie(future, promise) = Contract<NotReached, RejectValue>();
//   promise.Reject(from_here, std::move(value));
//   return future;
// }

}  // namespace base
