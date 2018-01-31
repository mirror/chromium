// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_ABSTRACT_PROMISE_H_
#define BASE_PROMISE_ABSTRACT_PROMISE_H_

#include <list>
#include <set>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "base/variant.h"

namespace base {

// Variant std::tuple and other templates can't contain void but they can
// contain the empty type Void.
struct Void {};

class Promises;

template <typename ResolveType, typename RejectType>
class Promise;

template <typename ResolveType, typename RejectType>
class PromiseResult;

template <typename ResolveType, typename RejectType>
class ManualPromiseResolver;

template <typename T>
struct Resolved {
  using type = T;
  T value;
};

template <>
struct Resolved<void> {
  using type = void;
  Void value;
};

template <typename T>
struct Rejected {
  using type = T;
  T value;
};

template <>
struct Rejected<void> {
  using type = void;
  Void value;
};

namespace internal {
template <typename Container, typename ContainerT>
struct AllContainerHelper;

template <typename ResolveT, typename RejectT>
struct ComputeThenPromise;

template <typename Container, typename ContainerT>
struct RaceContainerHelper;

template <typename T>
using ToNonVoidT = std::conditional_t<std::is_void<T>::value, Void, T>;

template <typename T>
using UndoToNonVoidT =
    std::conditional_t<std::is_same<Void, T>::value, void, T>;

// Treat Resolved<Variant...>> as a variant. This is legitimate since they have
// the same layout in memory.
template <typename... Ts>
struct IsVariant<Resolved<Variant<Ts...>>> {
  static constexpr bool value = true;
  static constexpr const VariantInfo* variant_info =
      &internal::VariantInfoGen<Ts...>::variant_info_;
};

// Treat Rejected<Variant...>> as a variant. This is legitimate since they have
// the same layout in memory.
template <typename... Ts>
struct IsVariant<Rejected<Variant<Ts...>>> {
  static constexpr bool value = true;
  static constexpr const VariantInfo* variant_info =
      &internal::VariantInfoGen<Ts...>::variant_info_;
};

template <typename T>
class UnwrapAliasType<Resolved<T>> {
 public:
  constexpr static const VariantTypeOps info =
      VariantTypeOps::Create<ToNonVoidT<T>>();
  constexpr static const VariantTypeOps* alias_info = &info;
};

template <typename T>
constexpr const VariantTypeOps UnwrapAliasType<Resolved<T>>::info;

template <typename T>
class UnwrapAliasType<Rejected<T>> {
 public:
  constexpr static const VariantTypeOps info =
      VariantTypeOps::Create<ToNonVoidT<T>>();
  constexpr static const VariantTypeOps* alias_info = &info;
};

template <typename T>
constexpr const VariantTypeOps UnwrapAliasType<Rejected<T>>::info;

class FinallyExecutor;

// Helper to automatically switch void to Void.
template <typename T>
struct SfinaeHelper {
  using type = void;
};

template <typename ResolveType, typename U = void>
struct PromiseTraits {
  using DefaultRejectType = void;
};

template <typename ResolveType>
struct PromiseTraits<
    ResolveType,
    typename SfinaeHelper<typename ResolveType::DefaultRejectType>::type> {
  using DefaultRejectType = typename ResolveType::DefaultRejectType;
};

class BASE_EXPORT AbstractPromise : public RefCounted<AbstractPromise> {
 public:
  AbstractPromise(const AbstractPromise&) = delete;
  AbstractPromise& operator=(const AbstractPromise&) = delete;

  enum class State {
    UNRESOLVED,
    RESOLVED_WITH_PROMISE,
    RESOLVED,
    REJECTED,
    AFTER_FINALLY,
    CANCELLED
  };

  State state() const { return state_; }

  class BASE_EXPORT Executor {
   public:
    virtual ~Executor() {}

    virtual void Execute(AbstractPromise* promise) = 0;
  };

  size_t creation_order() const { return creation_order_; }

  enum class PrerequisitePolicy { ALL, ANY, ALWAYS, NEVER, FINALLY };

  PrerequisitePolicy prerequisite_policy() const {
    return prerequisite_policy_;
  }

  AbstractVariant& value() {
    // If we're currying a promise, return it's value.
    if (value_.index() == 3)
      return Get<scoped_refptr<AbstractPromise>>(&value_)->value();
    return value_;
  }

  template <typename T>
  void ManualResolve(T t) {
    bool success = value().TryAssign(Resolved<T>{std::move(t)});
    DCHECK(success);
    OnManualResolve();
  }

  void ManualResolveVoid() {
    bool success = value().TryAssign(Resolved<void>());
    DCHECK(success);
    OnManualResolve();
  }

  template <typename T>
  void ManualReject(T t) {
    bool success = value().TryAssign(Rejected<T>{std::move(t)});
    DCHECK(success);
    OnManualResolve();
  }

  void ManualRejectVoid() {
    bool success = value().TryAssign(Rejected<void>());
    DCHECK(success);
    OnManualResolve();
  }

  const std::vector<scoped_refptr<AbstractPromise>>& prerequisites() const {
    return prerequisites_;
  }

  // Prevents this promise from running an executor, and cancels all dependent
  // promises unless they have PrerequisitePolicy::ANY, in which case that will
  // only be canceled if all of it's prerequisites are canceled.
  void Cancel();

  struct PromiseContext;

 private:
  friend class base::Promises;

  template <typename ResolveType, typename RejectType>
  friend class base::Promise;

  template <typename Container, typename ContainerT>
  friend struct AllContainerHelper;

  template <typename ResolveT, typename RejectT>
  friend struct ComputeThenPromise;

  template <typename Container, typename ContainerT>
  friend struct RaceContainerHelper;

  template <typename ResolveType, typename RejectType>
  struct ConstructWith {};

  friend class FinallyExecutor;

  friend class base::RefCounted<AbstractPromise>;

  template <typename ResolveType, typename RejectType>
  friend class base::ManualPromiseResolver;

  template <typename ResolveType, typename RejectType>
  AbstractPromise(ConstructWith<ResolveType, RejectType>,
                  Optional<Location> from_here,
                  scoped_refptr<SequencedTaskRunner> task_runner,
                  State state,
                  PrerequisitePolicy prerequisite_policy,
                  std::vector<scoped_refptr<AbstractPromise>> prerequisites,
                  std::unique_ptr<Executor> executor)
      : creation_order_(next_creation_order_++),
        from_here_(std::move(from_here)),
        task_runner_(std::move(task_runner)),
        state_(state),
        prerequisite_policy_(prerequisite_policy),
        prerequisites_(std::move(prerequisites)),
        value_(AbstractVariant::ConstructWith<
               Monostate,
               Resolved<ResolveType>,
               Rejected<RejectType>,
               scoped_refptr<internal::AbstractPromise>>{}),
        executor_(std::move(executor)) {
    InsertPrequisites();
  }

  virtual ~AbstractPromise();

  void InsertPrequisites();

  // Checks if this promise is ready to execute.
  bool CanExecute() const;

  // Calls Process() if CanExecute() returns true.
  void ExecuteIfPossible();

  static void Process();

  // Calls |RunExecutor()| or posts a task to do so if |from_here_| is not
  // nullopt.
  void Execute();

  void SetStateFromValue();

  void DetachFromPrerequistes();

  // Checks if any dependents can now execute.
  void OnManualResolve();

  // Returns the first canceled prerequisite if any or null if there isn't one.
  AbstractPromise* GetFirstRejectedPrerequisite();

  // Works out if we should cancel too if a prerequisite was canceled.
  bool ShouldCancelToo() const;

  bool IsInitialPromise() const;

  static size_t next_creation_order_;

  struct Comparator {
    bool operator()(const scoped_refptr<AbstractPromise>& op1,
                    const scoped_refptr<AbstractPromise>& op2) const;
  };

  const size_t creation_order_;
  const Optional<Location> from_here_;
  const scoped_refptr<SequencedTaskRunner> task_runner_;
  State state_ = State::UNRESOLVED;
  PrerequisitePolicy prerequisite_policy_ = PrerequisitePolicy::ALL;
  std::vector<scoped_refptr<AbstractPromise>> prerequisites_;
  std::set<scoped_refptr<AbstractPromise>, Comparator> dependants_;
  AbstractVariant value_;
  std::unique_ptr<Executor> executor_;
};

template <typename T>
class PromiseCallbackHelper {
 public:
  using Callback = base::OnceCallback<void(T)>;

  static base::OnceCallback<void(T)> GetResolveCallback(
      scoped_refptr<AbstractPromise>& promise) {
    return base::BindOnce(&AbstractPromise::ManualResolve<T>, promise);
  }

  static base::OnceCallback<void(T)> GetRejectCallback(
      scoped_refptr<AbstractPromise>& promise) {
    return base::BindOnce(&AbstractPromise::ManualReject<T>, promise);
  }
};

template <>
class PromiseCallbackHelper<void> {
 public:
  using Callback = base::OnceCallback<void()>;

  static base::OnceCallback<void()> GetResolveCallback(
      scoped_refptr<AbstractPromise>& promise) {
    return base::BindOnce(&AbstractPromise::ManualResolveVoid, promise);
  }

  static base::OnceCallback<void()> GetRejectCallback(
      scoped_refptr<AbstractPromise>& promise) {
    return base::BindOnce(&AbstractPromise::ManualRejectVoid, promise);
  }
};

// Helper to compute the Reject type given a promise and a Then/Catch which is
// the union of all Reject types.
template <typename T1, typename T2>
struct UnionOfTypes {
  using type = Variant<ToNonVoidT<T1>, ToNonVoidT<T2>>;
};

template <typename T>
struct UnionOfTypes<T, T> {
  using type = T;
};

template <typename T, typename... Ts>
struct UnionOfTypes<Variant<Ts...>, T> {
  static constexpr bool is_unique =
      VarArgIndexHelper<ToNonVoidT<T>, Ts...>::kIndex ==
      kVarArgIndexHelperInvalidIndex;
  using type = std::
      conditional_t<is_unique, Variant<Ts..., ToNonVoidT<T>>, Variant<Ts...>>;
};

template <typename T1, typename T2>
struct UnionOfVariants;

template <typename T, typename... Ts>
struct UnionOfVariants<Variant<Ts...>, Variant<T>> {
  using NonVoidT = ToNonVoidT<T>;
  static constexpr bool is_unique =
      VarArgIndexHelper<NonVoidT, Ts...>::kIndex ==
      kVarArgIndexHelperInvalidIndex;
  using type =
      std::conditional_t<is_unique, Variant<Ts..., NonVoidT>, Variant<Ts...>>;
};

template <typename... T1s, typename T, typename... T2s>
struct UnionOfVariants<Variant<T1s...>, Variant<T, T2s...>> {
  using NonVoidT = ToNonVoidT<T>;
  static constexpr bool is_unique =
      VarArgIndexHelper<NonVoidT, T1s...>::kIndex ==
      kVarArgIndexHelperInvalidIndex;
  static constexpr bool t2s_not_empty = sizeof...(T2s) > 0;
  using include =
      std::conditional_t<t2s_not_empty,
                         typename UnionOfVariants<Variant<T1s..., NonVoidT>,
                                                  Variant<T2s...>>::type,
                         Variant<T1s..., T>>;
  using exclude = std::conditional_t<
      t2s_not_empty,
      typename UnionOfVariants<Variant<T1s...>, Variant<T2s...>>::type,
      Variant<T1s...>>;
  using type = std::conditional_t<is_unique, include, exclude>;
};

template <typename T>
struct ConvertToNonVariantIfPossible;

template <typename T>
struct ConvertToNonVariantIfPossible<Variant<T>> {
  using type = T;
};

template <>
struct ConvertToNonVariantIfPossible<Variant<Void>> {
  using type = void;
};

template <typename... Ts>
struct ConvertToNonVariantIfPossible<Variant<Ts...>> {
  using type = Variant<Ts...>;
};

// Helper to compute a de-duplicated single type or Variant<> from a parameter
// pack.
template <typename... Ts>
struct UnionOfVarArgTypes;

template <typename Front, typename... Rest>
struct UnionOfVarArgTypes<Front, Rest...> {
  using type = typename ConvertToNonVariantIfPossible<
      typename UnionOfVariants<Variant<ToNonVoidT<Front>>,
                               Variant<Rest...>>::type>::type;
};

// Helper to unpack the types used by a callback.
template <typename R>
struct CallbackTypeHelper;

template <typename R, typename A>
struct CallbackTypeHelper<R(A)> {
  using ReturnT = R;
  using ArgT = std::decay_t<A>;
};

template <typename R, typename... A>
struct CallbackTypeHelper<R(A...)> {
  using ReturnT = R;
  using ArgT = std::tuple<A...>;
};

template <typename R>
struct CallbackTypeHelper<R()> {
  using ReturnT = R;
  using ArgT = void;
};

// Helper used to determine a Then or Catch's promise result.
template <typename ResolveT, typename RejectT>
struct ComputeThenPromise {
  using ThenResolve = ResolveT;
  using ThenReject = RejectT;
};

template <typename CurriedPromiseResolve,
          typename CurriedPromiseReject,
          typename RejectT>
struct ComputeThenPromise<Promise<CurriedPromiseResolve, CurriedPromiseReject>,
                          RejectT> {
  using ThenResolve = CurriedPromiseResolve;
  using ThenReject = CurriedPromiseReject;
};

template <typename ResolveResult, typename RejectResult, typename RejectT>
struct ComputeThenPromise<PromiseResult<ResolveResult, RejectResult>, RejectT> {
  using ThenResolve = ResolveResult;
  using ThenReject = typename UnionOfTypes<RejectT, RejectResult>::type;
};

// Helper used to extract the Nth AbstractPromise from a Promise<>
template <size_t N, typename...>
struct VariantPromiseHelper;

template <size_t N>
struct VariantPromiseHelper<N> {
  template <typename PromiseType>
  static internal::AbstractPromise* GetAbstractPromise(
      const PromiseType& promise) {
    NOTREACHED();
    return nullptr;
  }
};

template <size_t N, typename First, typename... Rest>
struct VariantPromiseHelper<N, First, Rest...> {
  template <typename VariantOfPromises>
  static internal::AbstractPromise* GetAbstractPromise(
      const VariantOfPromises& variant_of_promises) {
    if (N == variant_of_promises.index()) {
      return Get<First>(&variant_of_promises).abstract_promise_.get();
    } else {
      return VariantPromiseHelper<N + 1, Rest...>::GetAbstractPromise(
          variant_of_promises);
    }
  }
};

}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_ABSTRACT_PROMISE_H_
