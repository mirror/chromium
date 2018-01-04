// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_EXECUTION_HELPER_H_
#define BASE_PROMISE_PROMISE_EXECUTION_HELPER_H_

namespace base {

template <typename T>
class BASE_EXPORT PromiseResult;

namespace internal {

template <typename ReturnT, typename ArgT>
struct ExecutorHelper {
  using PromiseResultType = OnceCallback<PromiseResult<ReturnT>(ArgT)>;
  using Type = OnceCallback<ReturnT(ArgT)>;
};

template <typename ReturnT>
struct ExecutorHelper<ReturnT, void> {
  using PromiseResultType = OnceCallback<PromiseResult<ReturnT>()>;
  using Type = OnceCallback<ReturnT()>;
};

template <typename ArgT>
struct ExecutorHelper<void, ArgT> {
  using PromiseResultType = OnceCallback<PromiseResult<void>(ArgT)>;
  using Type = OnceCallback<void(ArgT)>;
};

template <>
struct ExecutorHelper<void, void> {
  using PromiseResultType = OnceCallback<PromiseResult<void>()>;
  using Type = OnceCallback<void()>;
};

template <typename ReturnT, typename T>
struct PromiseRunHelper {
  template <typename Arg>
  static std::enable_if_t<!std::is_void<Arg>::value, ReturnT> Run(
      PromiseT<T>* promise,
      OnceCallback<ReturnT(Arg)> callback) {
    return std::move(callback).Run(
        std::move(Get<std::decay_t<Arg>>(&promise->value())));
  }

  static ReturnT Run(PromiseT<T>* promise, OnceCallback<ReturnT()> callback) {
    return std::move(callback).Run();
  }
};

template <typename T>
struct PromiseRunHelper<void, T> {
  template <typename Arg>
  static std::enable_if_t<!std::is_void<Arg>::value, void> Run(
      PromiseT<T>* promise,
      OnceCallback<void(Arg)> callback) {
    std::move(callback).Run(
        std::move(Get<std::decay_t<Arg>>(&promise->value())));
  }

  static void Run(PromiseT<T>* promise, OnceCallback<void()> callback) {
    std::move(callback).Run();
  }
};

template <typename ReturnT, typename ArgT, typename OnceCallbackT>
struct PromiseHelper {
  static PromiseBase::ExecutorResult Execute(
      PromiseT<ArgT>* prerequsite,
      OnceCallbackT executor_callback,
      typename PromiseT<ReturnT>::ValueT* out_value) {
    out_value->template emplace<ReturnT>(PromiseRunHelper<ReturnT, ArgT>::Run(
        prerequsite, std::move(executor_callback)));
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  }
};

template <typename ReturnT, typename ArgT, typename OnceCallbackT>
struct PromiseHelper<PromiseResult<ReturnT>, ArgT, OnceCallbackT> {
  static PromiseBase::ExecutorResult Execute(
      PromiseT<ArgT>* prerequsite,
      OnceCallbackT executor_callback,
      typename PromiseT<ReturnT>::ValueT* out_value) {
    return ProcessResult(PromiseRunHelper<PromiseResult<ReturnT>, ArgT>::Run(
                             prerequsite, std::move(executor_callback)),
                         out_value);
  }

  static PromiseBase::ExecutorResult ProcessResult(
      ReturnT result,
      typename PromiseT<ReturnT>::ValueT* out_value) {
    return ProcessResult(PromiseResult<ReturnT>(std::move(result)), out_value);
  }

  static PromiseBase::ExecutorResult ProcessResult(
      PromiseResult<ReturnT> result,
      typename PromiseT<ReturnT>::ValueT* out_value) {
    // Could use std::visit here although the syntax is exquisitely awful.
    switch (result.value().index()) {
      case PromiseResult<ReturnT>::kResolvedIndex:
        *out_value = std::move(Get<ReturnT>(&result.value()));
        return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);

      case PromiseResult<ReturnT>::kRejectedIndex:
        *out_value = std::move(Get<RejectValue>(&result.value()));
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);

      case PromiseResult<ReturnT>::kResolvedWithPromiseIndex:
        return PromiseBase::ExecutorResult(
            Get<scoped_refptr<PromiseT<ReturnT>>>(&result.value()));

      default:
        NOTREACHED();
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);
    }
  };
};

template <typename ArgT, typename OnceCallbackT>
struct PromiseHelper<PromiseResult<void>, ArgT, OnceCallbackT> {
  static PromiseBase::ExecutorResult Execute(
      PromiseT<ArgT>* prerequsite,
      OnceCallbackT resolve_executor,
      typename PromiseT<void>::ValueT* out_value) {
    return ProcessResult(PromiseRunHelper<PromiseResult<void>, ArgT>::Run(
                             prerequsite, std::move(resolve_executor)),
                         out_value);
  }

  // ResultT is always PromiseResult<void> but if we use that here, the compiler
  // will complain because PromiseResult<> hasn't been defined yet. We could
  // define it in this file but for readability it's defined in promise.h which
  // requires this work around.
  template <typename ResultT>
  static PromiseBase::ExecutorResult ProcessResult(
      ResultT result,
      typename PromiseT<void>::ValueT* out_value) {
    // Could use std::visit here although the syntax is exquisitely awful.
    switch (result.value().index()) {
      case ResultT::kResolvedIndex:
        *out_value = Void();
        return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);

      case ResultT::kRejectedIndex:
        *out_value = std::move(Get<RejectValue>(&result.value()));
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);

      case ResultT::kResolvedWithPromiseIndex:
        return PromiseBase::ExecutorResult(
            Get<scoped_refptr<PromiseT<void>>>(&result.value()));

      default:
        NOTREACHED();
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);
    }
  };
};

template <typename ArgT, typename OnceCallbackT>
struct PromiseHelper<void, ArgT, OnceCallbackT> {
  static PromiseBase::ExecutorResult Execute(
      PromiseT<ArgT>* prerequsite,
      OnceCallbackT resolve_executor,
      typename PromiseT<void>::ValueT* out_value) {
    PromiseRunHelper<void, ArgT>::Run(prerequsite, std::move(resolve_executor));
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  };
};

}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_PROMISE_EXECUTION_HELPER_H_
