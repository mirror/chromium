// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_INTERNAL_H_
#define BASE_PROMISE_PROMISE_INTERNAL_H_

#include "base/promise/promise_base.h"
#include "base/promise/promise_helper.h"

namespace base {

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

  Optional<PromiseResolver<T>> GetResolver() override {
    return PromiseResolver<T>(this);
  }

  using NonVoidT = std::conditional_t<std::is_void<T>::value, Void, T>;

  void ResolveInternal(NonVoidT&& t) {
    PromiseT<T>::value_.emplace(typename PromiseT<T>::in_place_resolved_t(),
                                std::forward<NonVoidT>(t));
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED));
  }

  template <typename TT = T>
  std::enable_if_t<std::is_void<TT>::value, void> ResolveInternal() {
    PromiseT<void>::value_.emplace(
        typename PromiseT<void>::in_place_resolved_t());
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED));
  }

  void RejectInternal(RejectValue err) {
    PromiseT<T>::value_.emplace(typename PromiseT<T>::in_place_rejected_t(),
                                std::move(err));
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

// Helper for unwrapping the inner type of Promise<> and PromiseT<>.
// Note the reason for |non_void_type| is because Variant<> and std::tuple<>
// can not hold a void but they can hold the empty type Void.
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

// Helper which calls RacePromise::ExecuteT<>() for each prerequisite in turn
// with the right type, until a settled prerequisite is found.
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
  PromiseBase::State ExecuteT(int index) {
    using PromiseArg = typename UnwrapPromise<T>::non_void_type;
    using Promise = PromiseT<PromiseArg>;
    Promise* promise =
        static_cast<Promise*>(PromiseBase::prerequisites_[index].get());
    if (PromiseBase::prerequisites_[index]->state() ==
        PromiseBase::State::RESOLVED) {
      PromiseT<ReturnT>::value().emplace(
          typename PromiseT<ReturnT>::in_place_resolved_t(),
          std::move(Get<PromiseArg>(&promise->value())));
      return PromiseBase::State::RESOLVED;
    }
    if (PromiseBase::prerequisites_[index]->state() ==
        PromiseBase::State::REJECTED) {
      PromiseT<ReturnT>::value().emplace(
          typename PromiseT<ReturnT>::in_place_rejected_t(),
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

// Helper for deducing the correct index to emplace |T| into a Variant.
// There are two cases:
// 1. Variant<Ts...> contains T in which case kIndex is the index of |T| in
//    |Ts...| Used by promise::All or Race with a container of T.
// 2. Variant<Ts...> does not contain T in which case we use index 1 which
//    corresponds to a Promise variant's resolve type, which should be able to
//    emplace T. Used by promise::All or Race with a container of
//    Variant<Promise<Ts>...>
template <typename T, typename... Ts>
struct VariantEmplaceResolveHelper;

template <typename T, typename... Ts>
struct VariantEmplaceResolveHelper<T, Variant<Ts...>> {
  enum {
    kIndex =
        VarArgIndexHelper<T, Ts...>::kIndex == kVarArgIndexHelperInvalidIndex
            ? PromiseT<T>::kResolvedIndex
            : VarArgIndexHelper<T, Ts...>::kIndex
  };

  using IndexType = in_place_index_t<kIndex>;
};

// Helper for performing various operations on the Nth type of a
// Variant<Promise<T>...>
template <size_t N, typename...>
struct VariantPromiseHelper;

template <size_t N>
struct VariantPromiseHelper<N> {
  template <typename PromiseType>
  static PromiseBase* GetPromiseBase(const PromiseType& promise) {
    NOTREACHED();
    return nullptr;
  }

  template <typename VariantOfPromises, typename DestValue>
  static void MoveResolvedValue(VariantOfPromises& variant_of_promises,
                                size_t promise_variant_index,
                                DestValue* value) {
    NOTREACHED();
  }

  template <typename VariantOfPromises, typename DestValue>
  static void MoveRejectedValue(VariantOfPromises& variant_of_promises,
                                size_t promise_variant_index,
                                DestValue* value) {
    NOTREACHED();
  }
};  // namespace internal

template <size_t N, typename First, typename... Rest>
struct VariantPromiseHelper<N, First, Rest...> {
  template <typename VariantOfPromises>
  static PromiseBase* GetPromiseBase(
      const VariantOfPromises& variant_of_promises) {
    if (N == variant_of_promises.index()) {
      return Get<First>(&variant_of_promises).internal_promise_.get();
    } else {
      return VariantPromiseHelper<N + 1, Rest...>::GetPromiseBase(
          variant_of_promises);
    }
  }

  // Moves the resolved value out of |promise| into |destination|.
  template <typename DestinationT>
  static void MoveResolvedValue(PromiseBase* promise,
                                size_t promise_variant_index,
                                DestinationT* destination) {
    if (N == promise_variant_index) {
      using T = typename UnwrapPromise<First>::non_void_type;
      PromiseT<T>* promise_t = static_cast<PromiseT<T>*>(promise);
      // |destination| is a variant which either contains type |T| directly
      // (promise::All or Race with a container of T) or it's resolve value
      // (which is index 1) can accept T (promise::All or Race with a container
      // of Variant<Promise<Ts>...>).
      destination->emplace(
          typename VariantEmplaceResolveHelper<T, DestinationT>::IndexType(),
          std::move(Get<T>(&promise_t->value())));
    } else {
      VariantPromiseHelper<N + 1, Rest...>::MoveResolvedValue(
          promise, promise_variant_index, destination);
    }
  }

  // Moves the reject value out of |promise| into |destination|.
  template <typename DestinationT>
  static void MoveRejectedValue(PromiseBase* promise,
                                size_t promise_variant_index,
                                DestinationT* destination) {
    if (N == promise_variant_index) {
      using T = typename UnwrapPromise<First>::non_void_type;
      PromiseT<T>* promise_t = static_cast<PromiseT<T>*>(promise);
      *destination = std::move(Get<RejectValue>(&promise_t->value()));
    } else {
      VariantPromiseHelper<N + 1, Rest...>::MoveRejectedValue(
          promise, promise_variant_index, destination);
    }
  }
};

// Helper which lets ContainerRacePromise and ContainerAllPromise operate on
// container<T> and container<Variant<Ts...>>
template <typename T>
struct ContainerPromiseHelper;

template <typename T>
struct ContainerPromiseHelper<Promise<T>> {
  using NonVoidT = std::conditional_t<std::is_void<T>::value, Void, T>;
  using PromiseType = T;
  using ReturnType = NonVoidT;

  static PromiseBase* GetPromiseBase(const Promise<PromiseType>& promise) {
    return promise.internal_promise_.get();
  }

  static size_t GetPromiseVariantIndex(const Promise<PromiseType>& promise) {
    return 0;
  }

  // Moves the resolved value out of |promise| into |destination|.
  template <typename DestinationT>
  static void MoveResolvedValue(PromiseBase* promise,
                                size_t promise_type,
                                DestinationT* destination) {
    DCHECK_EQ(promise_type, 0u);
    *destination = std::move(
        Get<NonVoidT>(&static_cast<PromiseT<PromiseType>*>(promise)->value()));
  }

  // Moves the reject value out of |promise| into |destination|.
  template <typename DestinationT>
  static void MoveRejectedValue(PromiseBase* promise,
                                size_t promise_type,
                                DestinationT* destination) {
    DCHECK_EQ(promise_type, 0u);
    *destination = std::move(Get<RejectValue>(
        &static_cast<PromiseT<PromiseType>*>(promise)->value()));
  }
};

template <typename... Ts>
struct ContainerPromiseHelper<Variant<Ts...>> {
  using PromiseType = Variant<Ts...>;
  using ReturnType = Variant<typename UnwrapPromise<Ts>::non_void_type...>;

  static PromiseBase* GetPromiseBase(const PromiseType& variant_of_promises) {
    return VariantPromiseHelper<0, Ts...>::GetPromiseBase(variant_of_promises);
  }

  static size_t GetPromiseVariantIndex(const PromiseType& variant_of_promises) {
    return variant_of_promises.index();
  }

  // Moves the resolved value out of |promise| into |destination|.
  template <typename DestinationT>
  static void MoveResolvedValue(PromiseBase* promise,
                                size_t promise_variant_index,
                                DestinationT* destination) {
    return VariantPromiseHelper<0, Ts...>::MoveResolvedValue(
        promise, promise_variant_index, destination);
  }

  // Moves the reject value out of |promise| into |destination|.
  template <typename DestinationT>
  static void MoveRejectedValue(PromiseBase* promise,
                                size_t promise_variant_index,
                                DestinationT* destination) {
    return VariantPromiseHelper<0, Ts...>::MoveRejectedValue(
        promise, promise_variant_index, destination);
  }
};

// Implementation of promise::Race() with a container which returns a
// Promise<Variant<...>>
template <typename ContainerT>
class BASE_EXPORT ContainerRacePromise
    : public PromiseT<typename ContainerPromiseHelper<ContainerT>::ReturnType> {
 public:
  using ReturnT = typename ContainerPromiseHelper<ContainerT>::ReturnType;
  using PromiseType =
      typename internal::ContainerPromiseHelper<ContainerT>::PromiseType;

  ContainerRacePromise(std::vector<scoped_refptr<PromiseBase>> prerequisites,
                       std::vector<size_t> prerequisite_variant_indices)
      : PromiseT<ReturnT>(nullopt,
                          nullptr,
                          PromiseBase::PrerequisitePolicy::ANY,
                          std::move(prerequisites)),
        prerequisite_variant_indices_(std::move(prerequisite_variant_indices)) {
  }

  ContainerRacePromise(const ContainerRacePromise&) = delete;
  ContainerRacePromise& operator=(const ContainerRacePromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

 private:
  ~ContainerRacePromise() override {}

  ExecutorResult RunExecutor() override {
    for (size_t i = 0; i < PromiseBase::prerequisites_.size(); i++) {
      if (PromiseBase::prerequisites_[i]->state() ==
          PromiseBase::State::RESOLVED) {
        ContainerPromiseHelper<ContainerT>::MoveResolvedValue(
            PromiseBase::prerequisites_[i].get(),
            prerequisite_variant_indices_[i], &PromiseT<ReturnT>::value());
        return ExecutorResult(PromiseBase::State::RESOLVED);
      }
      if (PromiseBase::prerequisites_[i]->state() ==
          PromiseBase::State::REJECTED) {
        ContainerPromiseHelper<ContainerT>::MoveRejectedValue(
            PromiseBase::prerequisites_[i].get(),
            prerequisite_variant_indices_[i], &PromiseT<ReturnT>::value());
        return ExecutorResult(PromiseBase::State::REJECTED);
      }
    }
    NOTREACHED();
    return ExecutorResult(PromiseBase::State::UNRESOLVED);
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

  // We need to remember which variant index each prerequisite has in order to
  // correctly cast from PromiseBase to the Promise<T> in RunExecutor.
  std::vector<size_t> prerequisite_variant_indices_;
};

// Implementation of promise::All() with template var args promises.  Returns
// a Promise<std::tuple<...>> that resolves after all of it's prerequisites are
// resolved or rejects as soon as any reject.
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

// Implementation of promise::All() with a container which returns
// Promise<std:vector<Variant<...>>> that resolves after all of it's
// prerequisites are resolved or rejects as soon as any reject.
template <typename ContainerT>
class BASE_EXPORT ContainerAllPromise
    : public PromiseT<std::vector<
          typename ContainerPromiseHelper<ContainerT>::ReturnType>> {
 public:
  using ReturnT =
      std::vector<typename ContainerPromiseHelper<ContainerT>::ReturnType>;
  using PromiseType =
      typename internal::ContainerPromiseHelper<ContainerT>::PromiseType;

  ContainerAllPromise(std::vector<scoped_refptr<PromiseBase>> prerequisites,
                      std::vector<size_t> prerequisite_variant_indices)
      : PromiseT<ReturnT>(nullopt,
                          nullptr,
                          PromiseBase::PrerequisitePolicy::ALL,
                          std::move(prerequisites)),
        prerequisite_variant_indices_(std::move(prerequisite_variant_indices)) {
  }

  ContainerAllPromise(const ContainerAllPromise&) = delete;
  ContainerAllPromise& operator=(const ContainerAllPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

 private:
  ~ContainerAllPromise() override {}

  ExecutorResult RunExecutor() override {
    ReturnT vec(PromiseBase::prerequisites_.size());
    for (size_t i = 0; i < PromiseBase::prerequisites_.size(); i++) {
      if (PromiseBase::prerequisites_[i]->state() ==
          PromiseBase::State::RESOLVED) {
        ContainerPromiseHelper<ContainerT>::MoveResolvedValue(
            PromiseBase::prerequisites_[i].get(),
            prerequisite_variant_indices_[i], &vec[i]);
      } else if (PromiseBase::prerequisites_[i]->state() ==
                 PromiseBase::State::REJECTED) {
        ContainerPromiseHelper<ContainerT>::MoveRejectedValue(
            PromiseBase::prerequisites_[i].get(),
            prerequisite_variant_indices_[i], &PromiseT<ReturnT>::value());
        return ExecutorResult(PromiseBase::State::REJECTED);
      }
    }
    PromiseT<ReturnT>::value() = std::move(vec);
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

  // We need to remember which variant index each prerequisite has in order to
  // correctly cast from PromiseBase to the Promise<T> in RunExecutor.
  std::vector<size_t> prerequisite_variant_indices_;
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

// Helper for concatenating Variant parameter packs.
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

// Helper for deducing the return type of a promise. Note this could be removed
// if auto return types are allowed.
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
