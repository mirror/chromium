// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_EXECUTORS_H_
#define BASE_PROMISE_PROMISE_EXECUTORS_H_

#include "base/promise/abstract_promise.h"

namespace base {
namespace internal {

// Try to be smart about whether we pass promise executor arguments by value or
// by reference.
template <typename T>
using ToPromiseCallType =
    std::conditional_t<std::is_fundamental<T>::value ||
                           !std::is_copy_constructible<T>::value,
                       ToNonVoidT<T>,
                       const ToNonVoidT<T>&>;

template <typename ThenResolve, typename Arg>
struct ExecutorHelper {
  using ResolveOnceCallback = OnceCallback<ThenResolve(Arg)>;
  using RejectOnceCallback = OnceCallback<ThenResolve(ToPromiseCallType<Arg>)>;
};

template <typename Arg>
struct ExecutorHelper<void, Arg> {
  using ResolveOnceCallback = OnceCallback<void(Arg)>;
  using RejectOnceCallback = OnceCallback<void(ToPromiseCallType<Arg>)>;
};

template <typename ThenResolve>
struct ExecutorHelper<ThenResolve, void> {
  using ResolveOnceCallback = OnceCallback<ThenResolve()>;
  using RejectOnceCallback = OnceCallback<ThenResolve()>;
};

template <>
struct ExecutorHelper<void, void> {
  using ResolveOnceCallback = OnceCallback<void()>;
  using RejectOnceCallback = OnceCallback<void()>;
};

template <typename Result, typename RejectStorage>
struct AssignHelper {
  static void TryAssign(AbstractPromise* promise, Result result) {
    bool success =
        promise->value().TryAssign(Resolved<Result>{std::move(result)});
    DCHECK(success);
  }
};

// For executors which may reject, we need to deal with the situation where the
// storage type for rejected values is either Reject<Result> or
// Reject<Variant<T1, Result::RejectT...>>. The latter needs special handling
// because the executor function returns PromiseResult<ThenResolve, ThenReject>.
template <typename Resolve, typename Reject, typename RejectStorage>
struct AssignHelper<PromiseResult<Resolve, Reject>, RejectStorage> {
  constexpr static bool reject_is_variant =
      internal::IsVariant<RejectStorage>::value;

  template <bool can_enable = !reject_is_variant>
  static std::enable_if_t<can_enable, void> TryAssign(
      AbstractPromise* promise,
      PromiseResult<Resolve, Reject> result) {
    bool success = promise->value().TryAssign(result.value());
    DCHECK(success);
  }

  template <bool can_enable = reject_is_variant>
  static void TryAssign(std::enable_if_t<can_enable, AbstractPromise*> promise,
                        PromiseResult<Resolve, Reject> result) {
    switch (result.value().index()) {
      case 0: {
        bool success = promise->value().TryAssign(result.value());
        DCHECK(success);
        break;
      }
      case 1: {
        bool success = promise->value().TryAssign(
            RejectStorage{Get<Rejected<Reject>>(&result.value()).value});
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

template <typename Resolve, typename Reject, typename RejectStorage>
struct AssignHelper<Promise<Resolve, Reject>, RejectStorage> {
  static void TryAssign(AbstractPromise* promise,
                        Promise<Resolve, Reject> result) {
    bool success = promise->value().TryAssign(result.abstract_promise_);
    DCHECK(success);
  }
};

template <typename Callback, typename ArgType, typename RejectStorage>
struct RunHelper;

// Run helper for callbacks with a single argument.
template <typename CbResult,
          typename CbArg,
          typename ArgType,
          typename RejectStorage>
struct RunHelper<OnceCallback<CbResult(CbArg)>, ArgType, RejectStorage> {
  using Callback = OnceCallback<CbResult(CbArg)>;

  template <bool can_enable = !std::is_void<CbResult>::value>
  static std::enable_if_t<can_enable, void> Run(Callback& executor,
                                                AbstractPromise* arg,
                                                AbstractPromise* result) {
    if (executor.is_null()) {
      bool success = result->value().TryAssign(arg->value());
      DCHECK(success);
    } else {
      if (executor.IsCancelled()) {
        arg->Cancel();
        return;
      }
      AssignHelper<CbResult, RejectStorage>::TryAssign(
          result, std::move(executor).Run(
                      std::move(Get<ArgType>(&arg->value()).value)));
    }
  }

  template <bool can_enable = std::is_void<CbResult>::value>
  static void Run(std::enable_if_t<can_enable, Callback&> executor,
                  AbstractPromise* arg,
                  AbstractPromise* result) {
    bool success;
    if (executor.is_null()) {
      success = result->value().TryAssign(arg->value());
    } else {
      if (executor.IsCancelled()) {
        arg->Cancel();
        return;
      }
      std::move(executor).Run(std::move(Get<ArgType>(&arg->value()).value));
      success = result->value().TryAssign(Resolved<void>());
    }
    DCHECK(success);
  }
};

// Run helper for callbacks with no arguments.
template <typename CbResult, typename ArgType, typename RejectStorage>
struct RunHelper<OnceCallback<CbResult()>, ArgType, RejectStorage> {
  using Callback = OnceCallback<CbResult()>;

  template <bool can_enable = !std::is_void<CbResult>::value>
  static std::enable_if_t<can_enable, void> Run(Callback& executor,
                                                AbstractPromise* arg,
                                                AbstractPromise* result) {
    if (executor.is_null()) {
      bool success = result->value().TryAssign(arg->value());
      DCHECK(success);
    } else {
      if (executor.IsCancelled()) {
        arg->Cancel();
        return;
      }
      AssignHelper<CbResult, RejectStorage>::TryAssign(
          result, std::move(executor).Run());
    }
  }

  template <bool can_enable = std::is_void<CbResult>::value>
  static void Run(std::enable_if_t<can_enable, Callback&> executor,
                  AbstractPromise* arg,
                  AbstractPromise* result) {
    bool success;
    if (executor.is_null()) {
      success = result->value().TryAssign(arg->value());
    } else {
      if (executor.IsCancelled()) {
        arg->Cancel();
        return;
      }
      std::move(executor).Run();
      success = result->value().TryAssign(Resolved<void>());
    }
    DCHECK(success);
  }
};

// Run helper for running a callbacks with the arguments unpacked from a tuple.
template <typename CbResult, typename... CbArgs, typename RejectStorage>
struct RunHelper<OnceCallback<CbResult(CbArgs...)>,
                 Resolved<std::tuple<CbArgs...>>,
                 RejectStorage> {
  using Callback = OnceCallback<CbResult(CbArgs...)>;
  using ArgType = Resolved<std::tuple<CbArgs...>>;

  template <typename Callback, size_t... Indices>
  static void RunAndStoreResult(Callback& executor,
                                std::tuple<CbArgs...>& tuple,
                                AbstractPromise* result,
                                std::index_sequence<Indices...>) {
    AssignHelper<CbResult, RejectStorage>::TryAssign(
        result,
        std::move(executor).Run(std::move(std::get<Indices>(tuple))...));
  }

  template <typename Callback, size_t... Indices>
  static void RunAndStoreVoid(Callback& executor,
                              std::tuple<CbArgs...>& tuple,
                              AbstractPromise* result,
                              std::index_sequence<Indices...>) {
    std::move(executor).Run(std::move(std::get<Indices>(tuple))...);
    bool success = result->value().TryAssign(Resolved<void>());
    DCHECK(success);
  }

  template <bool can_enable = !std::is_void<CbResult>::value>
  static std::enable_if_t<can_enable, void> Run(Callback& executor,
                                                AbstractPromise* arg,
                                                AbstractPromise* result) {
    if (executor.is_null()) {
      bool success = result->value().TryAssign(arg->value());
      DCHECK(success);
    } else {
      if (executor.IsCancelled()) {
        arg->Cancel();
        return;
      }
      RunAndStoreResult(executor, Get<ArgType>(&arg->value()).value, result,
                        std::make_index_sequence<sizeof...(CbArgs)>{});
    }
  }

  template <bool can_enable = std::is_void<CbResult>::value>
  static void Run(std::enable_if_t<can_enable, Callback&> executor,
                  AbstractPromise* arg,
                  AbstractPromise* result) {
    if (executor.is_null()) {
      bool success = result->value().TryAssign(arg->value());
      DCHECK(success);
    } else {
      if (executor.IsCancelled()) {
        arg->Cancel();
        return;
      }
      RunAndStoreVoid(executor, Get<ArgType>(&arg->value()).value, result,
                      std::make_index_sequence<sizeof...(CbArgs)>{});
    }
  }
};

template <typename ResolveOnceCallback,
          typename RejectOnceCallback,
          typename ArgResolve,
          typename ArgReject,
          typename RejectStorage>
class AbstractPromiseExecutor : public AbstractPromise::Executor {
 public:
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
        RunHelper<ResolveOnceCallback, Resolved<ArgResolve>,
                  RejectStorage>::Run(resolve_executor_, prerequisite, promise);
        break;

      case AbstractPromise::State::REJECTED:
        RunHelper<RejectOnceCallback, Rejected<ArgReject>, RejectStorage>::Run(
            reject_executor_, prerequisite, promise);
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
