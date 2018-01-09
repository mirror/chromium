// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_EXECUTORS_H_
#define BASE_PROMISE_PROMISE_EXECUTORS_H_

#include "base/promise/abstract_promise.h"

namespace base {
namespace internal {

template <typename Result, typename Arg, typename RejectStorage>
class AbstractPromiseExecutor;

// For executors which may reject, we need to deal with the situation where the
// storage type for rejected values is either Reject<ThenReject> or
// Reject<Variant<T1, ThenReject...>>. The latter needs special handling because
// the executor function returns PromiseResult<ThenResolve, ThenReject>.
template <typename Result, typename ThenReject, typename RejectStorage>
struct BASE_EXPORT AssignHelper {
  constexpr static bool reject_is_variant =
      internal::IsVariant<RejectStorage>::value;

  template <bool can_enable = !reject_is_variant>
  static std::enable_if_t<can_enable, void> TryAssign(AbstractPromise* promise,
                                                      Result result) {
    bool success = promise->value().TryAssign(result.value());
    DCHECK(success);
  }

  template <bool can_enable = reject_is_variant>
  static void TryAssign(AbstractPromise* promise,
                        std::enable_if_t<can_enable, Result> result) {
    switch (result.value().index()) {
      case 0: {
        bool success = promise->value().TryAssign(result.value());
        DCHECK(success);
        break;
      }
      case 1: {
        bool success = promise->value().TryAssign(
            RejectStorage{Get<Rejected<ThenReject>>(&result.value()).value});
        DCHECK(success);
        break;
      }
      case 2: {
        bool success = promise->value().TryAssign(result.value());
        DCHECK(success);
        break;
      }
    }
  }
};

// Try to be smart about whether we pass promise executor argument by value or
// by reference.
template <typename T>
using ToPromiseCallType =
    std::conditional_t<std::is_fundamental<T>::value ||
                           !std::is_copy_constructible<T>::value,
                       ToNonVoidT<T>,
                       const ToNonVoidT<T>&>;

template <typename ThenResolve,
          typename ThenReject,
          typename ArgResolve,
          typename ArgReject,
          typename RejectStorage>
class BASE_EXPORT
    AbstractPromiseExecutor<PromiseResult<ThenResolve, ThenReject>,
                            Variant<ArgResolve, ArgReject>,
                            RejectStorage> : public AbstractPromise::Executor {
 public:
  using Result = PromiseResult<ThenResolve, ThenReject>;
  using ResolveOnceCallback = OnceCallback<Result(ToNonVoidT<ArgResolve>)>;
  using RejectOnceCallback = OnceCallback<Result(ToPromiseCallType<ArgReject>)>;

  AbstractPromiseExecutor(ResolveOnceCallback resolve_executor,
                          RejectOnceCallback reject_executor)
      : resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}
  ~AbstractPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    DCHECK_EQ(promise->prerequisites().size(), 1u);
    AbstractPromise* prerequisite = promise->prerequisites()[0].get();
    switch (prerequisite->state()) {
      case AbstractPromise::State::RESOLVED:
        if (resolve_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              std::move(resolve_executor_)
                  .Run(std::move(
                      Get<Resolved<ArgResolve>>(&prerequisite->value()).value))
                  .value());
        }
        break;

      case AbstractPromise::State::REJECTED:
        if (reject_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              std::move(reject_executor_)
                  .Run(std::move(
                      Get<Rejected<ArgReject>>(&prerequisite->value()).value))
                  .value());
        }
        break;
      default:
        DCHECK(false) << "Unexpected state";
    }
  }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

template <typename ThenResolve,
          typename ThenReject,
          typename ArgReject,
          typename RejectStorage>
class BASE_EXPORT
    AbstractPromiseExecutor<PromiseResult<ThenResolve, ThenReject>,
                            Variant<void, ArgReject>,
                            RejectStorage> : public AbstractPromise::Executor {
 public:
  using Result = PromiseResult<ThenResolve, ThenReject>;
  using ResolveOnceCallback = OnceCallback<Result()>;
  using RejectOnceCallback = OnceCallback<Result(ToPromiseCallType<ArgReject>)>;

  AbstractPromiseExecutor(ResolveOnceCallback resolve_executor,
                          RejectOnceCallback reject_executor)
      : resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}
  ~AbstractPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    DCHECK_EQ(promise->prerequisites().size(), 1u);
    AbstractPromise* prerequisite = promise->prerequisites()[0].get();
    switch (prerequisite->state()) {
      case AbstractPromise::State::RESOLVED:
        if (resolve_executor_.is_null()) {
          bool success = promise->value().TryAssign(Resolved<void>());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              std::move(resolve_executor_).Run());
        }
        break;

      case AbstractPromise::State::REJECTED:
        if (reject_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              std::move(reject_executor_)
                  .Run(std::move(
                      Get<Rejected<ArgReject>>(&prerequisite->value()).value))
                  .value());
        }
        break;
      default:
        DCHECK(false) << "Unexpected state";
    }
  }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

template <typename ThenResolve,
          typename ThenReject,
          typename ArgResolve,
          typename RejectStorage>
class BASE_EXPORT
    AbstractPromiseExecutor<PromiseResult<ThenResolve, ThenReject>,
                            Variant<ArgResolve, void>,
                            RejectStorage> : public AbstractPromise::Executor {
 public:
  using Result = PromiseResult<ThenResolve, ThenReject>;
  using ResolveOnceCallback = OnceCallback<Result(ToNonVoidT<ArgResolve>)>;
  using RejectOnceCallback = OnceCallback<Result()>;

  AbstractPromiseExecutor(ResolveOnceCallback resolve_executor,
                          RejectOnceCallback reject_executor)
      : resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}
  ~AbstractPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    DCHECK_EQ(promise->prerequisites().size(), 1u);
    AbstractPromise* prerequisite = promise->prerequisites()[0].get();
    switch (prerequisite->state()) {
      case AbstractPromise::State::RESOLVED:
        if (resolve_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              promise, std::move(resolve_executor_)
                           .Run(std::move(
                               Get<Resolved<ArgResolve>>(&prerequisite->value())
                                   .value)));
        }
        break;

      case AbstractPromise::State::REJECTED:
        if (reject_executor_.is_null()) {
          bool success = promise->value().TryAssign(Rejected<void>());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              promise, std::move(reject_executor_).Run());
        }
        break;
      default:
        DCHECK(false) << "Unexpected state";
    }
  }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

template <typename ThenResolve, typename ThenReject, typename RejectStorage>
class BASE_EXPORT
    AbstractPromiseExecutor<PromiseResult<ThenResolve, ThenReject>,
                            Variant<void, void>,
                            RejectStorage> : public AbstractPromise::Executor {
 public:
  using Result = PromiseResult<ThenResolve, ThenReject>;
  using ResolveOnceCallback = OnceCallback<Result()>;
  using RejectOnceCallback = OnceCallback<Result()>;

  AbstractPromiseExecutor(ResolveOnceCallback resolve_executor,
                          RejectOnceCallback reject_executor)
      : resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}
  ~AbstractPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    DCHECK_EQ(promise->prerequisites().size(), 1u);
    AbstractPromise* prerequisite = promise->prerequisites()[0].get();
    switch (prerequisite->state()) {
      case AbstractPromise::State::RESOLVED:
        if (resolve_executor_.is_null()) {
          bool success = promise->value().TryAssign(Resolved<void>());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              promise, std::move(resolve_executor_).Run());
        }
        break;

      case AbstractPromise::State::REJECTED:
        if (reject_executor_.is_null()) {
          bool success = promise->value().TryAssign(Rejected<void>());
          DCHECK(success);
        } else {
          AssignHelper<Result, ThenReject, RejectStorage>::TryAssign(
              promise, std::move(reject_executor_).Run());
        }
        break;
      default:
        DCHECK(false) << "Unexpected state";
    }
  }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

// This type of executor can only resolve.
template <typename ThenResolve, typename Arg>
struct ExecutorHelper {
  using ResolveOnceCallback = OnceCallback<ThenResolve(Arg)>;
  using RejectOnceCallback = OnceCallback<ThenResolve(ToPromiseCallType<Arg>)>;

  static void Resolve(AbstractPromise* promise,
                      AbstractPromise* prerequisite,
                      ResolveOnceCallback resolve_executor) {
    bool success = promise->value().TryAssign(Resolved<ThenResolve>{
        std::move(resolve_executor)
            .Run(std::move(Get<Resolved<Arg>>(&prerequisite->value()).value))});
    DCHECK(success);
  }

  static void Reject(AbstractPromise* promise,
                     AbstractPromise* prerequisite,
                     RejectOnceCallback reject_executor) {
    bool success = promise->value().TryAssign(Resolved<ThenResolve>{
        std::move(reject_executor)
            .Run(std::move(Get<Rejected<Arg>>(&prerequisite->value()).value))});
    DCHECK(success);
  }
};

template <typename Arg>
struct ExecutorHelper<void, Arg> {
  using ResolveOnceCallback = OnceCallback<void(Arg)>;
  using RejectOnceCallback = OnceCallback<void(ToPromiseCallType<Arg>)>;

  static void Resolve(AbstractPromise* promise,
                      AbstractPromise* prerequisite,
                      ResolveOnceCallback resolve_executor) {
    std::move(resolve_executor)
        .Run(std::move(Get<Resolved<Arg>>(&prerequisite->value()).value));
    bool success = promise->value().TryAssign(Resolved<void>());
    DCHECK(success);
  }

  static void Reject(AbstractPromise* promise,
                     AbstractPromise* prerequisite,
                     RejectOnceCallback reject_executor) {
    std::move(reject_executor)
        .Run(std::move(Get<Rejected<Arg>>(&prerequisite->value()).value));
    bool success = promise->value().TryAssign(Resolved<void>());
    DCHECK(success);
  }
};

template <typename ThenResolve>
struct ExecutorHelper<ThenResolve, void> {
  using ResolveOnceCallback = OnceCallback<ThenResolve(void)>;
  using RejectOnceCallback = OnceCallback<ThenResolve(void)>;

  static void Resolve(AbstractPromise* promise,
                      AbstractPromise* prerequisite,
                      ResolveOnceCallback resolve_executor) {
    bool success = promise->value().TryAssign(
        Resolved<ThenResolve>{std::move(resolve_executor).Run()});
    DCHECK(success);
  }

  static void Reject(AbstractPromise* promise,
                     AbstractPromise* prerequisite,
                     RejectOnceCallback reject_executor) {
    bool success = promise->value().TryAssign(
        Resolved<ThenResolve>{std::move(reject_executor).Run()});
    DCHECK(success);
  }
};

template <>
struct ExecutorHelper<void, void> {
  using ResolveOnceCallback = OnceCallback<void(void)>;
  using RejectOnceCallback = OnceCallback<void(void)>;

  static void Resolve(AbstractPromise* promise,
                      AbstractPromise* prerequisite,
                      ResolveOnceCallback resolve_executor) {
    std::move(resolve_executor).Run();
    bool success = promise->value().TryAssign(Resolved<void>());
    DCHECK(success);
  }

  static void Reject(AbstractPromise* promise,
                     AbstractPromise* prerequisite,
                     RejectOnceCallback reject_executor) {
    std::move(reject_executor).Run();
    bool success = promise->value().TryAssign(Resolved<void>());
    DCHECK(success);
  }
};

template <typename ThenResolve,
          typename ArgResolve,
          typename ArgReject,
          typename RejectStorage>
class BASE_EXPORT AbstractPromiseExecutor<ThenResolve,
                                          Variant<ArgResolve, ArgReject>,
                                          RejectStorage>
    : public AbstractPromise::Executor {
 public:
  using ResolveOnceCallback =
      typename ExecutorHelper<ThenResolve, ArgResolve>::ResolveOnceCallback;
  using RejectOnceCallback =
      typename ExecutorHelper<ThenResolve, ArgReject>::RejectOnceCallback;

  AbstractPromiseExecutor(ResolveOnceCallback resolve_executor,
                          RejectOnceCallback reject_executor)
      : resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  ~AbstractPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    DCHECK_EQ(promise->prerequisites().size(), 1u);
    AbstractPromise* prerequisite = promise->prerequisites()[0].get();
    switch (prerequisite->state()) {
      case AbstractPromise::State::RESOLVED:
        if (resolve_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          ExecutorHelper<ThenResolve, ArgResolve>::Resolve(
              promise, prerequisite, std::move(resolve_executor_));
        }
        break;

      case AbstractPromise::State::REJECTED:
        if (reject_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          ExecutorHelper<ThenResolve, ArgReject>::Reject(
              promise, prerequisite, std::move(reject_executor_));
        }
        break;
      default:
        DCHECK(false) << "Unexpected state";
    }
  }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

// This type of executor can only curry a promise.
template <typename ThenResolve, typename Arg>
struct CurryingExecutorHelper {
  using ResolveOnceCallback = OnceCallback<ThenResolve(Arg)>;
  using RejectOnceCallback = OnceCallback<ThenResolve(ToPromiseCallType<Arg>)>;

  static void Resolve(AbstractPromise* promise,
                      AbstractPromise* prerequisite,
                      ResolveOnceCallback resolve_executor) {
    bool success = promise->value().TryAssign(
        std::move(resolve_executor)
            .Run(std::move(Get<Resolved<Arg>>(&prerequisite->value()).value))
            .abstract_promise_);
    DCHECK(success);
  }

  static void Reject(AbstractPromise* promise,
                     AbstractPromise* prerequisite,
                     RejectOnceCallback reject_executor) {
    bool success = promise->value().TryAssign(
        std::move(reject_executor)
            .Run(std::move(Get<Rejected<Arg>>(&prerequisite->value()).value))
            .abstract_promise_);
    DCHECK(success);
  }
};

template <typename ThenResolve>
struct CurryingExecutorHelper<ThenResolve, void> {
  using ResolveOnceCallback = OnceCallback<ThenResolve(void)>;
  using RejectOnceCallback = OnceCallback<ThenResolve(void)>;

  static void Resolve(AbstractPromise* promise,
                      AbstractPromise* prerequisite,
                      ResolveOnceCallback resolve_executor) {
    bool success = promise->value().TryAssign(
        std::move(resolve_executor).Run().abstract_promise_);
    DCHECK(success);
  }

  static void Reject(AbstractPromise* promise,
                     AbstractPromise* prerequisite,
                     RejectOnceCallback reject_executor) {
    bool success = promise->value().TryAssign(
        std::move(reject_executor).Run().abstract_promise_);
    DCHECK(success);
  }
};

template <typename ThenResolve,
          typename ThenReject,
          typename ArgResolve,
          typename ArgReject,
          typename RejectStorage>
class BASE_EXPORT AbstractPromiseExecutor<Promise<ThenResolve, ThenReject>,
                                          Variant<ArgResolve, ArgReject>,
                                          RejectStorage>
    : public AbstractPromise::Executor {
 public:
  using ReturnPromise = Promise<ThenResolve, ThenReject>;
  using ResolveOnceCallback =
      typename ExecutorHelper<ReturnPromise, ArgResolve>::ResolveOnceCallback;
  using RejectOnceCallback =
      typename ExecutorHelper<ReturnPromise, ArgReject>::RejectOnceCallback;

  AbstractPromiseExecutor(ResolveOnceCallback resolve_executor,
                          RejectOnceCallback reject_executor)
      : resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  ~AbstractPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    DCHECK_EQ(promise->prerequisites().size(), 1u);
    AbstractPromise* prerequisite = promise->prerequisites()[0].get();
    switch (prerequisite->state()) {
      case AbstractPromise::State::RESOLVED:
        if (resolve_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          CurryingExecutorHelper<ReturnPromise, ArgResolve>::Resolve(
              promise, prerequisite, std::move(resolve_executor_));
        }
        break;

      case AbstractPromise::State::REJECTED:
        if (reject_executor_.is_null()) {
          bool success = promise->value().TryAssign(prerequisite->value());
          DCHECK(success);
        } else {
          CurryingExecutorHelper<ReturnPromise, ArgReject>::Reject(
              promise, prerequisite, std::move(reject_executor_));
        }
        break;
      default:
        DCHECK(false) << "Unexpected state";
    }
  }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

class BASE_EXPORT FinallyExecutor : public AbstractPromise::Executor {
 public:
  explicit FinallyExecutor(OnceClosure finally_callback);

  ~FinallyExecutor() override;

  void Execute(AbstractPromise* promise) override;

 private:
  OnceClosure finally_callback_;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_PROMISE_EXECUTORS_H_
