// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_INTERNAL_H_
#define BASE_PROMISE_PROMISE_INTERNAL_H_

#include "base/promise/promise_base.h"
#include "base/promise/promise_helper.h"

namespace base {

template <typename T>
class BASE_EXPORT PromiseResolver;

namespace internal {
template <typename T>
class BASE_EXPORT PromiseT;
}  // namespace internal

namespace internal {

template <typename T>
class BASE_EXPORT InitialPromise : public PromiseT<T> {
 public:
  InitialPromise() {}

  InitialPromise(const InitialPromise&) = delete;
  InitialPromise& operator=(const InitialPromise&) = delete;

  void ResolveInternal(T&& t) {
    PromiseT<T>::value_.emplace(in_place_index_t<1>(), std::forward<T>(t));
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED));
  }

  void RejectInternal(RejectValue err) {
    PromiseT<T>::value_.emplace(in_place_index_t<2>(), std::move(err));
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::REJECTED));
  }

 protected:
  bool HasOnResolveExecutor() override { return false; }

  bool HasOnRejectExecutor() override { return false; }

  const RejectValue* GetRejectValue() const override {
    return &Get<RejectValue>(&PromiseT<T>::value());
  }

  void PropagateRejectValue(const RejectValue& reject_reason) override {
    NOTREACHED();
  }

  ~InitialPromise() override {}
};

template <>
class BASE_EXPORT InitialPromise<void> : public PromiseT<void> {
 public:
  InitialPromise() {}

  InitialPromise(const InitialPromise&) = delete;
  InitialPromise& operator=(const InitialPromise&) = delete;

  void ResolveInternal() {
    PromiseT<void>::value_.emplace(in_place_index_t<1>());
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED));
  }

  void RejectInternal(RejectValue err) {
    PromiseT<void>::value_.emplace(in_place_index_t<2>(), std::move(err));
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::REJECTED));
  }

 protected:
  bool HasOnResolveExecutor() override { return false; }

  bool HasOnRejectExecutor() override { return false; }

  const RejectValue* GetRejectValue() const override {
    return &Get<RejectValue>(&PromiseT<void>::value());
  }

  void PropagateRejectValue(const RejectValue& reject_reason) override {
    NOTREACHED();
  }

  ~InitialPromise() override {}
};

// A Then() or a Catch() which returns a PromiseResult<ReturnT>.
template <typename ReturnT, typename ArgT>
class BASE_EXPORT ChainedPromise : public PromiseT<ReturnT> {
 public:
  using ResolveOnceCallback =
      typename ExecutorHelper<ReturnT, ArgT>::PromiseResultType;
  using RejectOnceCallback =
      typename ExecutorHelper<ReturnT, const RejectValue&>::PromiseResultType;

  ChainedPromise(Optional<Location> from_here,
                 scoped_refptr<SequencedTaskRunner> task_runner,
                 scoped_refptr<PromiseBase> prerequisite,
                 ResolveOnceCallback resolve_executor,
                 RejectOnceCallback reject_executor)
      : PromiseT<ReturnT>(std::move(from_here),
                          std::move(task_runner),
                          PromiseBase::PrerequisitePolicy::ALL,
                          {std::move(prerequisite)}),
        resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  ChainedPromise(const ChainedPromise&) = delete;
  ChainedPromise& operator=(const ChainedPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

 private:
  ~ChainedPromise() override {}

  ExecutorResult RunExecutor() override {
    PromiseT<ArgT>* prerequsite =
        static_cast<PromiseT<ArgT>*>(PromiseBase::prerequisites_[0].get());
    if (PromiseBase::state() == State::DEPENDENCY_REJECTED) {
      return PromiseHelper<PromiseResult<ReturnT>, ArgT, RejectOnceCallback>::
          Execute(prerequsite, std::move(reject_executor_),
                  &PromiseT<ReturnT>::value());
    }
    return PromiseHelper<PromiseResult<ReturnT>, ArgT, ResolveOnceCallback>::
        Execute(prerequsite, std::move(resolve_executor_),
                &PromiseT<ReturnT>::value());
  }

  bool HasOnResolveExecutor() override { return !resolve_executor_.is_null(); }

  bool HasOnRejectExecutor() override { return !reject_executor_.is_null(); }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

// A Then() or a Catch() which returns a plain return type and can't reject.
template <typename ReturnT, typename ArgT>
class BASE_EXPORT NoRejectChainedPromise : public PromiseT<ReturnT> {
 public:
  using ResolveOnceCallback = typename ExecutorHelper<ReturnT, ArgT>::Type;
  using RejectOnceCallback =
      typename ExecutorHelper<ReturnT, const RejectValue&>::Type;

  NoRejectChainedPromise(Optional<Location> from_here,
                         scoped_refptr<SequencedTaskRunner> task_runner,
                         scoped_refptr<PromiseBase> prerequisite,
                         ResolveOnceCallback resolve_executor,
                         RejectOnceCallback reject_executor)
      : PromiseT<ReturnT>(std::move(from_here),
                          std::move(task_runner),
                          PromiseBase::PrerequisitePolicy::ALL,
                          {std::move(prerequisite)}),
        resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  NoRejectChainedPromise(const NoRejectChainedPromise&) = delete;
  NoRejectChainedPromise& operator=(const NoRejectChainedPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

 private:
  ~NoRejectChainedPromise() override {}

  ExecutorResult RunExecutor() override {
    PromiseT<ArgT>* prerequsite =
        static_cast<PromiseT<ArgT>*>(PromiseBase::prerequisites_[0].get());
    if (PromiseBase::state() == State::DEPENDENCY_REJECTED) {
      return PromiseHelper<ReturnT, ArgT, RejectOnceCallback>::Execute(
          prerequsite, std::move(reject_executor_),
          &PromiseT<ReturnT>::value());
    }
    return PromiseHelper<ReturnT, ArgT, ResolveOnceCallback>::Execute(
        prerequsite, std::move(resolve_executor_), &PromiseT<ReturnT>::value());
  }
  bool HasOnResolveExecutor() override { return !resolve_executor_.is_null(); }

  bool HasOnRejectExecutor() override { return !reject_executor_.is_null(); }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

// A Then() or a Catch() which returns a Promise the value of which is curried
// as the result of the CurringPromise.
template <typename ReturnT, typename ArgT>
class BASE_EXPORT CurringPromise : public PromiseT<ReturnT> {
 public:
  using ResolveOnceCallback =
      typename ExecutorHelper<Promise<ReturnT>, ArgT>::Type;
  using RejectOnceCallback =
      typename ExecutorHelper<Promise<ReturnT>, const RejectValue&>::Type;

  CurringPromise(Optional<Location> from_here,
                 scoped_refptr<SequencedTaskRunner> task_runner,
                 scoped_refptr<PromiseBase> prerequisite,
                 ResolveOnceCallback resolve_executor,
                 RejectOnceCallback reject_executor)
      : PromiseT<ReturnT>(std::move(from_here),
                          std::move(task_runner),
                          PromiseBase::PrerequisitePolicy::ALL,
                          {std::move(prerequisite)}),
        resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  CurringPromise(const CurringPromise&) = delete;
  CurringPromise& operator=(const CurringPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;
  using ValueT = typename PromiseT<ReturnT>::ValueT;

  const ValueT& value() const override { return curried_promise_->value(); }

  ValueT& value() override { return curried_promise_->value(); }

 private:
  ~CurringPromise() override {}

  const RejectValue* GetRejectValue() const override {
    return &Get<RejectValue>(&curried_promise_->value());
  }

  ExecutorResult RunExecutor() override {
    PromiseT<ArgT>* prerequsite =
        static_cast<PromiseT<ArgT>*>(PromiseBase::prerequisites_[0].get());
    if (PromiseBase::state() == State::DEPENDENCY_REJECTED) {
      curried_promise_ = PromiseRunHelper<Promise<ReturnT>, ArgT>::Run(
                             prerequsite, std::move(reject_executor_))
                             .internal_promise_;
      return PromiseBase::ExecutorResult(curried_promise_);
    }

    curried_promise_ = PromiseRunHelper<Promise<ReturnT>, ArgT>::Run(
                           prerequsite, std::move(resolve_executor_))
                           .internal_promise_;
    return PromiseBase::ExecutorResult(curried_promise_);
  }

  bool HasOnResolveExecutor() override { return !resolve_executor_.is_null(); }

  bool HasOnRejectExecutor() override { return !reject_executor_.is_null(); }

  scoped_refptr<PromiseT<ReturnT>> curried_promise_;
  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

template <typename T>
struct UnwrapPromise;

template <typename T>
struct UnwrapPromise<Promise<T>> {
  using type = T;
  using non_void_type = T;
};

template <>
struct UnwrapPromise<Promise<void>> {
  using type = void;
  using non_void_type = Void;
};

template <typename T>
struct UnwrapPromise<PromiseT<T>> {
  using type = T;
  using non_void_type = T;
};

template <>
struct UnwrapPromise<PromiseT<void>> {
  using type = void;
  using non_void_type = Void;
};

template <typename... Rest>
struct RacePromiseHelper;

template <>
struct RacePromiseHelper<> {
  template <typename RacePromiseT>
  static PromiseBase::State Execute(RacePromiseT* race_promise, int index) {
    NOTREACHED();
    return PromiseBase::State::REJECTED;
  }
};

template <typename First, typename... Rest>
struct RacePromiseHelper<First, Rest...> {
  template <typename RacePromiseT>
  static PromiseBase::State Execute(RacePromiseT* race_promise, int index) {
    PromiseBase::State state = race_promise->template ExecuteT<First>(index);
    if (state != PromiseBase::State::UNRESOLVED)
      return state;
    return RacePromiseHelper<Rest...>::Execute(race_promise, index + 1);
  }
};

// A promise that resolves or rejects after any of it's prerequisites resolves
// or rejects.
template <typename ReturnT, typename... Promises>
class BASE_EXPORT RacePromise : public PromiseT<ReturnT> {
 public:
  RacePromise(std::vector<scoped_refptr<PromiseBase>> prerequisites)
      : PromiseT<ReturnT>(nullopt,
                          nullptr,
                          PromiseBase::PrerequisitePolicy::ANY,
                          std::move(prerequisites)) {}

  RacePromise(const RacePromise&) = delete;
  RacePromise& operator=(const RacePromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

  template <typename T>
  PromiseBase::State ExecuteT(int N) {
    using PromiseArg = typename UnwrapPromise<T>::non_void_type;
    using Promise = PromiseT<PromiseArg>;
    Promise* promise =
        static_cast<Promise*>(PromiseBase::prerequisites_[N].get());
    if (PromiseBase::prerequisites_[N]->state() ==
        PromiseBase::State::RESOLVED) {
      PromiseT<ReturnT>::value().emplace(
          in_place_index_t<1>(), std::move(Get<PromiseArg>(&promise->value())));
      return PromiseBase::State::RESOLVED;
    }
    if (PromiseBase::prerequisites_[N]->state() ==
        PromiseBase::State::REJECTED) {
      PromiseT<ReturnT>::value().emplace(
          in_place_index_t<2>(),
          std::move(Get<RejectValue>(&promise->value())));
      return PromiseBase::State::REJECTED;
    }
    return PromiseBase::State::UNRESOLVED;
  }

 private:
  ~RacePromise() override {}

  ExecutorResult RunExecutor() override {
    return ExecutorResult(RacePromiseHelper<Promises...>::Execute(this, 0));
  }

  bool HasOnResolveExecutor() override { return true; }

  bool HasOnRejectExecutor() override { return true; }

  const RejectValue* GetRejectValue() const override {
    NOTREACHED();
    return nullptr;
  }

  void PropagateRejectValue(const RejectValue& reject_reason) override {
    NOTREACHED();
  }
};

// A promise that resolves or rejects after all of it's prerequisites resolves
// or any reject.
template <typename ReturnT, typename... Promises>
class BASE_EXPORT AllPromise : public PromiseT<ReturnT> {
 public:
  AllPromise(std::vector<scoped_refptr<PromiseBase>> prerequisites)
      : PromiseT<ReturnT>(nullopt,
                          nullptr,
                          PromiseBase::PrerequisitePolicy::ALL,
                          std::move(prerequisites)) {}

  AllPromise(const AllPromise&) = delete;
  AllPromise& operator=(const AllPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

 private:
  ~AllPromise() override {}

  template <std::size_t I>
  typename UnwrapPromise<TypeAtHelper<I, Promises...>>::non_void_type
  GetPromiseValue() {
    using PromiseType =
        typename UnwrapPromise<TypeAtHelper<I, Promises...>>::non_void_type;
    PromiseT<PromiseType>* prerequisite = static_cast<PromiseT<PromiseType>*>(
        PromiseBase::prerequisites_[I].get());
    DCHECK_EQ(prerequisite->value().index(), 1u);
    return std::move(Get<PromiseType>(&prerequisite->value()));
  }

  template <std::size_t... I>
  ReturnT MakeReturnT(std::index_sequence<I...>) {
    return ReturnT(GetPromiseValue<I>()...);
  }

  ExecutorResult RunExecutor() override {
    if (PromiseBase::state() == State::DEPENDENCY_REJECTED) {
      for (auto& prerequisite : PromiseBase::prerequisites_) {
        if (prerequisite->state() == State::REJECTED) {
          PromiseT<ReturnT>::value().template emplace<RejectValue>(
              prerequisite->GetRejectValue()->Clone());
        }
      }
      return ExecutorResult(PromiseBase::State::REJECTED);
    }

    PromiseT<ReturnT>::value().template emplace<ReturnT>(
        MakeReturnT(std::make_index_sequence<sizeof...(Promises)>{}));
    return ExecutorResult(PromiseBase::State::RESOLVED);
  }

  bool HasOnResolveExecutor() override { return true; }

  bool HasOnRejectExecutor() override { return true; }

  const RejectValue* GetRejectValue() const override {
    NOTREACHED();
    return nullptr;
  }

  void PropagateRejectValue(const RejectValue& reject_reason) override {
    NOTREACHED();
  }
};

// A promise that runs last after all Then and Catch promises.
class BASE_EXPORT FinallyPromise : public PromiseBase {
 public:
  FinallyPromise(Optional<Location> from_here,
                 scoped_refptr<SequencedTaskRunner> task_runner,
                 scoped_refptr<PromiseBase> prerequisite,
                 OnceClosure finally_callback);

  bool HasOnResolveExecutor() override;

  bool HasOnRejectExecutor() override;

  const RejectValue* GetRejectValue() const override;

  void PropagateRejectValue(const RejectValue& reject_reason) override;

  ExecutorResult RunExecutor() override;

 private:
  ~FinallyPromise() override;

  OnceClosure finally_callback_;
};

template <typename... T>
struct CatVariant {
  using type = Variant<T...>;
};

template <typename... As, typename... Bs>
struct CatVariant<Variant<As...>, Variant<Bs...>> {
  using type = Variant<As..., Bs...>;
};

// Helper used by promise::Race to remove duplicate types from a Variant<...>
template <typename...>
struct DedupVariant;

template <>
struct DedupVariant<> {
  using type = Variant<>;
};

template <typename Front, typename... Rest>
struct DedupVariant<Front, Rest...> {
  using front = Variant<Front>;
  using rest = typename DedupVariant<Rest...>::type;
  using combined = typename CatVariant<front, rest>::type;
  static constexpr bool isUnique =
      VarArgIndexHelper<Front, Rest...>::kIndex == -1;
  using type = typename std::conditional<isUnique, combined, rest>::type;
};

// Helper for deducing the returns type of a promise. Note this could be removed
// if auto is allowed.
template <typename R>
struct PromiseReturnTypeHelper {
  using type = Promise<R>;
};

template <typename R>
struct PromiseReturnTypeHelper<Promise<R>> {
  using type = Promise<R>;
};

template <typename R>
struct PromiseReturnTypeHelper<PromiseResult<R>> {
  using type = Promise<R>;
};

template <typename R>
using PromiseReturnType = typename PromiseReturnTypeHelper<R>::type;

}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_PROMISE_INTERNAL_H_
