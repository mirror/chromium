// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_ALL_PROMISE_EXECUTOR_H_
#define BASE_PROMISE_ALL_PROMISE_EXECUTOR_H_

#include "base/promise/abstract_promise.h"

namespace base {
namespace internal {

template <size_t I, typename Tuple>
std::tuple_element_t<I, Tuple>& GetResolvedValueFromPromise(
    const std::vector<scoped_refptr<AbstractPromise>>& promises) {
  using ResolvedType =
      base::Resolved<UndoToNonVoidT<std::tuple_element_t<I, Tuple>>>;
  return Get<ResolvedType>(&promises[I]->value()).value;
}

class TupleConstructor {
 public:
  virtual ~TupleConstructor() {}

  virtual void ConstructTuple(
      const std::vector<scoped_refptr<AbstractPromise>>& promises,
      AbstractVariant* result) = 0;
};

template <typename Tuple,
          typename Indices =
              std::make_index_sequence<std::tuple_size<Tuple>::value>>
class TupleConstructorImpl;

template <typename Tuple, size_t... Indices>
class TupleConstructorImpl<Tuple, std::index_sequence<Indices...>>
    : public TupleConstructor {
 public:
  TupleConstructorImpl() {}
  ~TupleConstructorImpl() override {}

  void ConstructTuple(
      const std::vector<scoped_refptr<AbstractPromise>>& promises,
      AbstractVariant* result) override {
    bool success = result->TryAssign(Resolved<Tuple>{Tuple(
        std::move(GetResolvedValueFromPromise<Indices, Tuple>(promises))...)});
    DCHECK(success);
  }
};

template <typename T>
class AllContainerPromiseExecutor : public AbstractPromise::Executor {
 public:
  AllContainerPromiseExecutor() {}
  ~AllContainerPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    for (const auto& prerequisite : promise->prerequisites()) {
      if (prerequisite->state() == AbstractPromise::State::REJECTED) {
        bool success = promise->value().TryAssignOrUnwrapAndEmplace(
            prerequisite->value(), 2);
        DCHECK(success);
        return;
      }
    }

    Resolved<std::vector<ToNonVoidT<T>>> result;
    result.value.reserve(promise->prerequisites().size());
    for (const auto& prerequisite : promise->prerequisites()) {
      DCHECK_EQ(prerequisite->state(), AbstractPromise::State::RESOLVED);
      result.value.push_back(
          std::move(Get<Resolved<T>>(&prerequisite->value())).value);
    }
    bool success = promise->value().TryAssign(std::move(result));
    DCHECK(success);
  }
};

template <typename VectorT>
class AllVariantContainerPromiseExecutor : public AbstractPromise::Executor {
 public:
  AllVariantContainerPromiseExecutor() {}
  ~AllVariantContainerPromiseExecutor() override {}

  void Execute(AbstractPromise* promise) override {
    for (const auto& prerequisite : promise->prerequisites()) {
      if (prerequisite->state() == AbstractPromise::State::REJECTED) {
        bool success = promise->value().TryAssignOrUnwrapAndEmplace(
            prerequisite->value(), 2);
        DCHECK(success);
        return;
      }
    }

    Resolved<VectorT> result{VectorT(promise->prerequisites().size())};
    for (size_t i = 0; i < promise->prerequisites().size(); i++) {
      DCHECK_EQ(promise->prerequisites()[i]->state(),
                AbstractPromise::State::RESOLVED);
      bool success = result.value[i].TryUnwrapAndAssign(
          1, promise->prerequisites()[i]->value());
      DCHECK(success);
    }
    bool success = promise->value().TryAssign(std::move(result));
    DCHECK(success);
  }
};

// Helper for constructing All promises.
template <typename Container, typename ContainerT>
struct AllContainerHelper;

template <typename Container, typename Resolve, typename Reject>
struct AllContainerHelper<Container, Promise<Resolve, Reject>> {
  using PromiseResolve = std::vector<ToNonVoidT<Resolve>>;
  using PromiseType = Promise<PromiseResolve, Reject>;

  static PromiseType All(const Container& promises) {
    std::vector<scoped_refptr<AbstractPromise>> prerequistes;
    prerequistes.reserve(promises.size());
    for (typename Container::const_iterator it = promises.begin();
         it != promises.end(); ++it) {
      prerequistes.push_back(it->abstract_promise_);
    }
    scoped_refptr<AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new AbstractPromise(
            AbstractPromise::ConstructWith<PromiseResolve, Reject>(), nullopt,
            nullptr, AbstractPromise::State::UNRESOLVED,
            AbstractPromise::PrerequisitePolicy::ALL, std::move(prerequistes),
            std::make_unique<AllContainerPromiseExecutor<Resolve>>()),
        AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return PromiseType(std::move(promise));
  }
};

template <typename Container, typename... Promises>
struct AllContainerHelper<Container, Variant<Promises...>> {
  using PromiseResolve = std::vector<
      typename UnionOfVarArgTypes<typename Promises::ResolveT...>::type>;
  using PromiseReject =
      typename UnionOfVarArgTypes<typename Promises::RejectT...>::type;
  using PromiseType = Promise<PromiseResolve, PromiseReject>;

  static PromiseType All(const Container& promises) {
    std::vector<scoped_refptr<AbstractPromise>> prerequistes;
    prerequistes.reserve(promises.size());
    for (typename Container::const_iterator it = promises.begin();
         it != promises.end(); ++it) {
      prerequistes.push_back(
          VariantPromiseHelper<0, Promises...>::GetAbstractPromise(*it));
    }
    scoped_refptr<AbstractPromise> promise(subtle::AdoptRefIfNeeded(
        new AbstractPromise(
            AbstractPromise::ConstructWith<PromiseResolve, PromiseReject>(),
            nullopt, nullptr, AbstractPromise::State::UNRESOLVED,
            AbstractPromise::PrerequisitePolicy::ALL, std::move(prerequistes),
            std::make_unique<
                AllVariantContainerPromiseExecutor<PromiseResolve>>()),
        AbstractPromise::kRefCountPreference));
    promise->ExecuteIfPossible();
    return PromiseType(std::move(promise));
  }
};

class BASE_EXPORT AllPromiseExecutor : public AbstractPromise::Executor {
 public:
  explicit AllPromiseExecutor(
      std::unique_ptr<TupleConstructor> tuple_constructor);

  ~AllPromiseExecutor() override;

  template <typename Tuple>
  static std::unique_ptr<AllPromiseExecutor> Create() {
    return std::make_unique<AllPromiseExecutor>(
        std::make_unique<TupleConstructorImpl<Tuple>>());
  }

 private:
  void Execute(AbstractPromise* promise) override;

  std::unique_ptr<TupleConstructor> tuple_constructor_;
};
}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_RACE_PROMISE_EXECUTOR_H_
