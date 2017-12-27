// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_INTERNAL_H_
#define BASE_PROMISE_PROMISE_INTERNAL_H_

#include "base/promise/promise_base.h"
#include "base/variant.h"

namespace base {

template <typename T>
class BASE_EXPORT Promise;

struct Void {};

// Helper types which can disambiguate Resolve & Reject when they have the same
// type, but allow auto conversion when they're distinct for terser syntax.
template <typename T>
class BASE_EXPORT Resolve {
 public:
  constexpr Resolve(T t) : value_(std::move(t)) {}

  template <class... Args>
  constexpr explicit Resolve(in_place_t, Args&&... args)
      : value_(std::forward<Args>(args)...) {}

  constexpr Resolve(Resolve&& other) = default;

  ~Resolve() {}

  Resolve(const Resolve&) = delete;
  Resolve& operator=(const Resolve&) = delete;

  constexpr T& operator*() { return value_; }

  constexpr const T& operator*() const { return value_; }

  constexpr T* operator->() { return &value_; }

  constexpr const T* operator->() const { return &value_; }

 private:
  T value_;
};

template <>
class BASE_EXPORT Resolve<void> {
 public:
  constexpr Resolve() {}

  Resolve(Resolve&& other) = default;

  Resolve(const Resolve&) = delete;
  Resolve& operator=(const Resolve&) = delete;
};

template <typename T>
class BASE_EXPORT Reject {
 public:
  constexpr Reject(T t) : value_(std::move(t)) {}

  template <class... Args>
  constexpr explicit Reject(in_place_t, Args&&... args)
      : value_(std::forward<Args>(args)...) {}

  constexpr Reject(Reject&& other) = default;

  Reject(const Reject&) = delete;
  Reject& operator=(const Reject&) = delete;

  constexpr T& operator*() { return value_; }

  constexpr const T& operator*() const { return value_; }

  constexpr T* operator->() { return &value_; }

  constexpr const T* operator->() const { return &value_; }

 private:
  T value_;
};

template <typename T>
class BASE_EXPORT InternalPromise : public PromiseBase {
 public:
  using ValueT = Variant<Monostate, Resolve<T>, Reject<Value>>;

  virtual const ValueT& value() const { return value_; };
  virtual ValueT& value() { return value_; };

  InternalPromise(Resolve<T> resolve)
      : PromiseBase(nullopt,
                    nullptr,
                    State::RESOLVED,
                    PrerequisitePolicy::NEVER,
                    {}),
        value_(in_place_index_t<1>(), std::move(resolve)) {}

  InternalPromise(Reject<Value> reject)
      : PromiseBase(nullopt,
                    nullptr,
                    State::REJECTED,
                    PrerequisitePolicy::NEVER,
                    {}),
        value_(in_place_index_t<2>(), std::move(reject)) {}

 protected:
  InternalPromise() {}

  InternalPromise(Optional<Location> from_here,
                  scoped_refptr<SequencedTaskRunner> task_runner,
                  PrerequisitePolicy prerequisite_policy,
                  std::vector<scoped_refptr<PromiseBase>> prerequisites)
      : PromiseBase(std::move(from_here),
                    std::move(task_runner),
                    PromiseBase::State::UNRESOLVED,
                    prerequisite_policy,
                    std::move(prerequisites)) {}

  ~InternalPromise() override {}

  InternalPromise(const InternalPromise&) = delete;
  InternalPromise& operator=(const InternalPromise&) = delete;

  const Value* GetRejectValue() const override {
    return &*Get<Reject<Value>>(&value_);
  }

  void PropagateRejectValue(const Value& reject_reason) override {
    value_.emplace(in_place_index_t<2>(), reject_reason.Clone());
  }

  bool HasOnResolveExecutor() override { return false; }

  bool HasOnRejectExecutor() override { return false; }

  ValueT value_;
};

template <typename T>
class BASE_EXPORT InitialPromise : public InternalPromise<T> {
 public:
  InitialPromise() {}

  InitialPromise(const InitialPromise&) = delete;
  InitialPromise& operator=(const InitialPromise&) = delete;

  void ResolveInternal(T&& t) {
    InternalPromise<T>::value_.emplace(in_place_index_t<1>(),
                                       std::forward<T>(t));
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED));
  }

  void RejectInternal(Value err) {
    InternalPromise<T>::value_.emplace(in_place_index_t<2>(), std::move(err));
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::REJECTED));
  }

 protected:
  bool HasOnResolveExecutor() override { return false; }

  bool HasOnRejectExecutor() override { return false; }

  const Value* GetRejectValue() const override {
    return &*Get<Reject<Value>>(&InternalPromise<T>::value());
  }

  void PropagateRejectValue(const Value& reject_reason) override {
    NOTREACHED();
  }

  ~InitialPromise() override {}
};

template <>
class BASE_EXPORT InitialPromise<void> : public InternalPromise<void> {
 public:
  InitialPromise() {}

  InitialPromise(const InitialPromise&) = delete;
  InitialPromise& operator=(const InitialPromise&) = delete;

  void ResolveInternal() {
    InternalPromise<void>::value_.emplace(in_place_index_t<1>());
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED));
  }

  void RejectInternal(Value err) {
    InternalPromise<void>::value_.emplace(in_place_index_t<2>(),
                                          std::move(err));
    PromiseBase::OnManualResolve(
        PromiseBase::ExecutorResult(PromiseBase::State::REJECTED));
  }

 protected:
  bool HasOnResolveExecutor() override { return false; }

  bool HasOnRejectExecutor() override { return false; }

  const Value* GetRejectValue() const override {
    return &*Get<Reject<Value>>(&InternalPromise<void>::value());
  }

  void PropagateRejectValue(const Value& reject_reason) override {
    NOTREACHED();
  }

  ~InitialPromise() override {}
};

template <typename T>
class BASE_EXPORT PromiseResolver {
 public:
  PromiseResolver() {}

  explicit PromiseResolver(scoped_refptr<InitialPromise<T>> promise)
      : promise_(std::move(promise)) {}

  void resolve(T&& t) { promise_->ResolveInternal(std::forward<T>(t)); }

  void reject(Value err) { promise_->RejectInternal(std::move(err)); }

  scoped_refptr<InitialPromise<T>> promise_;
};

template <>
class BASE_EXPORT PromiseResolver<void> {
 public:
  PromiseResolver() {}

  explicit PromiseResolver(scoped_refptr<InitialPromise<void>> promise)
      : promise_(std::move(promise)) {}

  void resolve() { promise_->ResolveInternal(); }

  void reject(Value err) { promise_->RejectInternal(std::move(err)); }

  scoped_refptr<InitialPromise<void>> promise_;
};

template <typename T>
struct BASE_EXPORT PromiseResult {
  PromiseResult(Resolve<T> r) : value(std::move(r)) {}

  PromiseResult(Reject<Value> err) : value(std::move(err)) {}

  PromiseResult(Promise<T> chained_promise)
      : value(std::move(chained_promise.internal_promise_)) {}

  PromiseResult(PromiseResult&& other) = default;

  PromiseResult(const PromiseResult&) = delete;
  PromiseResult& operator=(const PromiseResult&) = delete;

  Variant<Resolve<T>, Reject<Value>, scoped_refptr<InternalPromise<T>>> value;
};

namespace {
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

template <typename ReturnT, typename ArgT, typename OnceCallbackT>
struct ExecutionInnerHelper {
  static ReturnT ExecuteResolve(InternalPromise<ArgT>* prerequsite,
                                OnceCallbackT resolve_executor) {
    return std::move(resolve_executor)
        .Run(std::move(*Get<Resolve<ArgT>>(&prerequsite->value())));
  }

  static ReturnT ExecuteReject(InternalPromise<ArgT>* prerequsite,
                               OnceCallbackT reject_executor) {
    return std::move(reject_executor)
        .Run(std::move(*Get<Reject<Value>>(&prerequsite->value())));
  }
};

template <typename ReturnT, typename OnceCallbackT>
struct ExecutionInnerHelper<ReturnT, void, OnceCallbackT> {
  static ReturnT ExecuteResolve(InternalPromise<void>* prerequsite,
                                OnceCallbackT resolve_executor) {
    return std::move(resolve_executor).Run();
  }

  static ReturnT ExecuteReject(InternalPromise<void>* prerequsite,
                               OnceCallbackT reject_executor) {
    return std::move(reject_executor)
        .Run(std::move(*Get<Reject<Value>>(&prerequsite->value())));
  }
};

template <typename ReturnT, typename ArgT, typename OnceCallbackT>
struct ExecutionHelper {
  static PromiseBase::ExecutorResult ExecuteResolve(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT resolve_executor,
      typename InternalPromise<ReturnT>::ValueT* out_value) {
    out_value->template emplace<Resolve<ReturnT>>(
        ExecutionInnerHelper<ReturnT, ArgT, OnceCallbackT>::ExecuteResolve(
            prerequsite, std::move(resolve_executor)));
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  }

  static PromiseBase::ExecutorResult ExecuteReject(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT reject_executor,
      typename InternalPromise<ReturnT>::ValueT* out_value) {
    ReturnT foo =
        ExecutionInnerHelper<ReturnT, ArgT, OnceCallbackT>::ExecuteReject(
            prerequsite, std::move(reject_executor));
    out_value->template emplace<Resolve<ReturnT>>(foo);
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  };
};

template <typename ReturnT, typename ArgT, typename OnceCallbackT>
struct ExecutionHelper<PromiseResult<ReturnT>, ArgT, OnceCallbackT> {
  static PromiseBase::ExecutorResult ExecuteResolve(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT resolve_executor,
      typename InternalPromise<ReturnT>::ValueT* out_value) {
    return ProcessResult(
        ExecutionInnerHelper<PromiseResult<ReturnT>, ArgT, OnceCallbackT>::
            ExecuteResolve(prerequsite, std::move(resolve_executor)),
        out_value);
  }

  static PromiseBase::ExecutorResult ExecuteReject(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT reject_executor,
      typename InternalPromise<ReturnT>::ValueT* out_value) {
    return ProcessResult(
        ExecutionInnerHelper<PromiseResult<ReturnT>, ArgT, OnceCallbackT>::
            ExecuteReject(prerequsite, std::move(reject_executor)),
        out_value);
  };

  static PromiseBase::ExecutorResult ProcessResult(
      ReturnT result,
      typename InternalPromise<ReturnT>::ValueT* out_value) {
    return ProcessResult(PromiseResult<ReturnT>(std::move(result)), out_value);
  }

  static PromiseBase::ExecutorResult ProcessResult(
      PromiseResult<ReturnT> result,
      typename InternalPromise<ReturnT>::ValueT* out_value) {
    // Could use std::visit here although the syntax is exquisitely awful.
    switch (result.value.index()) {
      case 0:
        *out_value = std::move(Get<Resolve<ReturnT>>(&result.value));
        return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);

      case 1:
        *out_value = std::move(Get<Reject<Value>>(&result.value));
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);

      case 2:
        return PromiseBase::ExecutorResult(
            Get<scoped_refptr<InternalPromise<ReturnT>>>(&result.value));

      default:
        NOTREACHED();
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);
    }
  };
};

template <typename ArgT, typename OnceCallbackT>
struct ExecutionHelper<PromiseResult<void>, ArgT, OnceCallbackT> {
  static PromiseBase::ExecutorResult ExecuteResolve(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT resolve_executor,
      typename InternalPromise<void>::ValueT* out_value) {
    return ProcessResult(
        ExecutionInnerHelper<PromiseResult<void>, ArgT, OnceCallbackT>::
            ExecuteResolve(prerequsite, std::move(resolve_executor)),
        out_value);
  }

  static PromiseBase::ExecutorResult ExecuteReject(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT reject_executor,
      typename InternalPromise<void>::ValueT* out_value) {
    return ProcessResult(
        ExecutionInnerHelper<PromiseResult<void>, ArgT, OnceCallbackT>::
            ExecuteReject(prerequsite, std::move(reject_executor)),
        out_value);
  };

  static PromiseBase::ExecutorResult ProcessResult(
      PromiseResult<void> result,
      typename InternalPromise<void>::ValueT* out_value) {
    // Could use std::visit here although the syntax is exquisitely awful.
    switch (result.value.index()) {
      case 0:
        *out_value = Resolve<void>();
        return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);

      case 1:
        *out_value = std::move(Get<Reject<Value>>(&result.value));
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);

      case 2:
        return PromiseBase::ExecutorResult(
            Get<scoped_refptr<InternalPromise<void>>>(&result.value));

      default:
        NOTREACHED();
        return PromiseBase::ExecutorResult(PromiseBase::State::REJECTED);
    }
  };
};

template <typename ArgT, typename OnceCallbackT>
struct ExecutionHelper<void, ArgT, OnceCallbackT> {
  static PromiseBase::ExecutorResult ExecuteResolve(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT resolve_executor,
      typename InternalPromise<void>::ValueT* out_value) {
    std::move(resolve_executor)
        .Run(std::move(*Get<Resolve<ArgT>>(&prerequsite->value())));
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  };

  static PromiseBase::ExecutorResult ExecuteReject(
      InternalPromise<ArgT>* prerequsite,
      OnceCallbackT reject_executor,
      typename InternalPromise<void>::ValueT* out_value) {
    std::move(reject_executor)
        .Run(std::move(*Get<Reject<Value>>(&prerequsite->value())));
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  };
};

template <typename OnceCallbackT>
struct ExecutionHelper<void, void, OnceCallbackT> {
  static PromiseBase::ExecutorResult ExecuteResolve(
      InternalPromise<void>* prerequsite,
      OnceCallbackT resolve_executor,
      typename InternalPromise<void>::ValueT* out_value) {
    std::move(resolve_executor).Run();
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  };

  static PromiseBase::ExecutorResult ExecuteReject(
      InternalPromise<void>* prerequsite,
      OnceCallbackT reject_executor,
      typename InternalPromise<void>::ValueT* out_value) {
    std::move(reject_executor)
        .Run(std::move(*Get<Reject<Value>>(&prerequsite->value())));
    return PromiseBase::ExecutorResult(PromiseBase::State::RESOLVED);
  };
};

}  // namespace

template <typename ReturnT, typename ArgT>
class BASE_EXPORT ChainedPromise : public InternalPromise<ReturnT> {
 public:
  using ResolveOnceCallback =
      typename ExecutorHelper<ReturnT, ArgT>::PromiseResultType;
  using RejectOnceCallback =
      typename ExecutorHelper<ReturnT, const Value&>::PromiseResultType;

  ChainedPromise(Optional<Location> from_here,
                 scoped_refptr<SequencedTaskRunner> task_runner,
                 std::vector<scoped_refptr<PromiseBase>> prerequisites,
                 ResolveOnceCallback resolve_executor,
                 RejectOnceCallback reject_executor)
      : InternalPromise<ReturnT>(std::move(from_here),
                                 std::move(task_runner),
                                 PromiseBase::PrerequisitePolicy::ALL,
                                 std::move(prerequisites)),
        resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  ChainedPromise(const ChainedPromise&) = delete;
  ChainedPromise& operator=(const ChainedPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

 private:
  ~ChainedPromise() override {}

  ExecutorResult RunExecutor() override {
    InternalPromise<ArgT>* prerequsite = static_cast<InternalPromise<ArgT>*>(
        PromiseBase::prerequisites_[0].get());
    if (PromiseBase::state() == State::DEPENDENCY_REJECTED) {
      return ExecutionHelper<PromiseResult<ReturnT>, ArgT, RejectOnceCallback>::
          ExecuteReject(prerequsite, std::move(reject_executor_),
                        &InternalPromise<ReturnT>::value());
    }
    return ExecutionHelper<PromiseResult<ReturnT>, ArgT, ResolveOnceCallback>::
        ExecuteResolve(prerequsite, std::move(resolve_executor_),
                       &InternalPromise<ReturnT>::value());
  }

  bool HasOnResolveExecutor() override { return !resolve_executor_.is_null(); }

  bool HasOnRejectExecutor() override { return !reject_executor_.is_null(); }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

template <typename ReturnT, typename ArgT>
class BASE_EXPORT NoRejectChainedPromise : public InternalPromise<ReturnT> {
 public:
  using ResolveOnceCallback = typename ExecutorHelper<ReturnT, ArgT>::Type;
  using RejectOnceCallback =
      typename ExecutorHelper<ReturnT, const Value&>::Type;

  NoRejectChainedPromise(Optional<Location> from_here,
                         scoped_refptr<SequencedTaskRunner> task_runner,
                         std::vector<scoped_refptr<PromiseBase>> prerequisites,
                         ResolveOnceCallback resolve_executor,
                         RejectOnceCallback reject_executor)
      : InternalPromise<ReturnT>(std::move(from_here),
                                 std::move(task_runner),
                                 PromiseBase::PrerequisitePolicy::ALL,
                                 std::move(prerequisites)),
        resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  NoRejectChainedPromise(const NoRejectChainedPromise&) = delete;
  NoRejectChainedPromise& operator=(const NoRejectChainedPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

 private:
  ~NoRejectChainedPromise() override {}

  ExecutorResult RunExecutor() override {
    InternalPromise<ArgT>* prerequsite = static_cast<InternalPromise<ArgT>*>(
        PromiseBase::prerequisites_[0].get());
    if (PromiseBase::state() == State::DEPENDENCY_REJECTED) {
      return ExecutionHelper<ReturnT, ArgT, RejectOnceCallback>::ExecuteReject(
          prerequsite, std::move(reject_executor_),
          &InternalPromise<ReturnT>::value());
    }
    return ExecutionHelper<ReturnT, ArgT, ResolveOnceCallback>::ExecuteResolve(
        prerequsite, std::move(resolve_executor_),
        &InternalPromise<ReturnT>::value());
  }
  bool HasOnResolveExecutor() override { return !resolve_executor_.is_null(); }

  bool HasOnRejectExecutor() override { return !reject_executor_.is_null(); }

  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

template <typename ReturnT, typename ArgT>
class BASE_EXPORT CurringPromise : public InternalPromise<ReturnT> {
 public:
  using ResolveOnceCallback =
      typename ExecutorHelper<Promise<ReturnT>, ArgT>::Type;
  using RejectOnceCallback =
      typename ExecutorHelper<Promise<ReturnT>, const Value&>::Type;

  CurringPromise(Optional<Location> from_here,
                 scoped_refptr<SequencedTaskRunner> task_runner,
                 std::vector<scoped_refptr<PromiseBase>> prerequisites,
                 ResolveOnceCallback resolve_executor,
                 RejectOnceCallback reject_executor)
      : InternalPromise<ReturnT>(std::move(from_here),
                                 std::move(task_runner),
                                 PromiseBase::PrerequisitePolicy::ALL,
                                 std::move(prerequisites)),
        resolve_executor_(std::move(resolve_executor)),
        reject_executor_(std::move(reject_executor)) {}

  CurringPromise(const CurringPromise&) = delete;
  CurringPromise& operator=(const CurringPromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;
  using ValueT = typename InternalPromise<ReturnT>::ValueT;

  const ValueT& value() const override { return curried_promise_->value(); }

  ValueT& value() override { return curried_promise_->value(); }

 private:
  ~CurringPromise() override {}

  const Value* GetRejectValue() const override {
    return &*Get<Reject<Value>>(&curried_promise_->value());
  }

  ExecutorResult RunExecutor() override {
    InternalPromise<ArgT>* prerequsite = static_cast<InternalPromise<ArgT>*>(
        PromiseBase::prerequisites_[0].get());
    if (PromiseBase::state() == State::DEPENDENCY_REJECTED) {
      curried_promise_ =
          ExecutionInnerHelper<Promise<ReturnT>, ArgT, RejectOnceCallback>::
              ExecuteReject(prerequsite, std::move(reject_executor_))
                  .internal_promise_;
      return PromiseBase::ExecutorResult(curried_promise_);
    }

    curried_promise_ =
        ExecutionInnerHelper<Promise<ReturnT>, ArgT, ResolveOnceCallback>::
            ExecuteResolve(prerequsite, std::move(resolve_executor_))
                .internal_promise_;
    return PromiseBase::ExecutorResult(curried_promise_);
  }

  bool HasOnResolveExecutor() override { return !resolve_executor_.is_null(); }

  bool HasOnRejectExecutor() override { return !reject_executor_.is_null(); }

  scoped_refptr<InternalPromise<ReturnT>> curried_promise_;
  ResolveOnceCallback resolve_executor_;
  RejectOnceCallback reject_executor_;
};

template <typename T>
struct UnwrapPromise;

template <typename T>
struct UnwrapPromise<Promise<T>> {
  using type = T;
};

template <typename T>
struct UnwrapPromise<InternalPromise<T>> {
  using type = T;
};

template <typename... Rest>
struct RacePromiseHelper;

template <typename First>
struct RacePromiseHelper<First> {
  template <typename RacePromiseT>
  static PromiseBase::State Execute(RacePromiseT* race_promise, int index) {
    return race_promise->template ExecuteT<First>(index);
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

template <typename ReturnT, typename... Promises>
class BASE_EXPORT RacePromise : public InternalPromise<ReturnT> {
 public:
  RacePromise(std::vector<scoped_refptr<PromiseBase>> prerequisites)
      : InternalPromise<ReturnT>(nullopt,
                                 nullptr,
                                 PromiseBase::PrerequisitePolicy::ANY,
                                 std::move(prerequisites)) {}

  RacePromise(const RacePromise&) = delete;
  RacePromise& operator=(const RacePromise&) = delete;

  using ExecutorResult = PromiseBase::ExecutorResult;
  using State = PromiseBase::State;

  template <typename T>
  PromiseBase::State ExecuteT(int N) {
    using PromiseArg = typename UnwrapPromise<T>::type;
    using Promise = InternalPromise<PromiseArg>;
    Promise* promise =
        static_cast<Promise*>(PromiseBase::prerequisites_[N].get());
    if (PromiseBase::prerequisites_[N]->state() ==
        PromiseBase::State::RESOLVED) {
      InternalPromise<ReturnT>::value().emplace(
          in_place_index_t<1>(), in_place_t(),
          std::move(*Get<Resolve<PromiseArg>>(&promise->value())));
      return PromiseBase::State::RESOLVED;
    }
    if (PromiseBase::prerequisites_[N]->state() ==
        PromiseBase::State::REJECTED) {
      InternalPromise<ReturnT>::value().emplace(
          in_place_index_t<2>(), in_place_t(),
          std::move(*Get<Reject<Value>>(&promise->value())));
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

  const Value* GetRejectValue() const override {
    NOTREACHED();
    return nullptr;
  }

  void PropagateRejectValue(const Value& reject_reason) override {
    NOTREACHED();
  }
};

template <typename ReturnT, typename... Promises>
class BASE_EXPORT AllPromise : public InternalPromise<ReturnT> {
 public:
  AllPromise(std::vector<scoped_refptr<PromiseBase>> prerequisites)
      : InternalPromise<ReturnT>(nullopt,
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
  auto GetPromiseVariant() {
    using PromiseType =
        typename UnwrapPromise<TypeAtHelper<I, Promises...>>::type;
    using ReturnType = Variant<Resolve<PromiseType>, Reject<Value>>;
    InternalPromise<PromiseType>* prerequisite =
        static_cast<InternalPromise<PromiseType>*>(
            PromiseBase::prerequisites_[I].get());
    switch (prerequisite->value().index()) {
      case 1:
        return ReturnType(
            std::move(Get<Resolve<PromiseType>>(&prerequisite->value())));

      case 2:
        return ReturnType(
            std::move(Get<Reject<Value>>(&prerequisite->value())));

      default:
        NOTREACHED();
        return ReturnType(Reject<Value>(Value()));
    };
  }

  template <std::size_t... I>
  ReturnT MakeReturnT(std::index_sequence<I...>) {
    return ReturnT(GetPromiseVariant<I>()...);
  }

  ExecutorResult RunExecutor() override {
    InternalPromise<ReturnT>::value().template emplace<Resolve<ReturnT>>(
        MakeReturnT(std::make_index_sequence<sizeof...(Promises)>{}));
    return ExecutorResult(PromiseBase::State::RESOLVED);
  }

  bool HasOnResolveExecutor() override { return true; }

  bool HasOnRejectExecutor() override { return true; }

  const Value* GetRejectValue() const override {
    NOTREACHED();
    return nullptr;
  }

  void PropagateRejectValue(const Value& reject_reason) override {
    NOTREACHED();
  }
};

class BASE_EXPORT FinallyPromise : public PromiseBase {
 public:
  FinallyPromise(Optional<Location> from_here,
                 scoped_refptr<SequencedTaskRunner> task_runner,
                 std::vector<scoped_refptr<PromiseBase>> prerequisites,
                 OnceClosure finally_callback);

  bool HasOnResolveExecutor() override;

  bool HasOnRejectExecutor() override;

  const Value* GetRejectValue() const override;

  void PropagateRejectValue(const Value& reject_reason) override;

  ExecutorResult RunExecutor() override;

 private:
  ~FinallyPromise() override;

  OnceClosure finally_callback_;
};

namespace {
template <typename... T>
struct CatVariant {
  using type = Variant<T...>;
};

template <typename... As, typename... Bs>
struct CatVariant<Variant<As...>, Variant<Bs...>> {
  using type = Variant<As..., Bs...>;
};

template <typename...>
struct DedupAndRemoveVoid;

template <typename Front>
struct DedupAndRemoveVoid<Front> {
  static constexpr bool isVoid = std::is_void<Front>::value;
  using type =
      typename std::conditional<!isVoid, Variant<Front>, Variant<>>::type;
};

template <typename Front, typename... Rest>
struct DedupAndRemoveVoid<Front, Rest...> {
  using front = Variant<Front>;
  using rest = typename DedupAndRemoveVoid<Rest...>::type;
  using combined = typename CatVariant<front, rest>::type;
  static constexpr bool isVoid = std::is_void<Front>::value;
  static constexpr bool isUnique =
      VarArgIndexHelper<Front, Rest...>::kIndex == -1;
  using type =
      typename std::conditional<!isVoid && isUnique, combined, rest>::type;
};
}  // namespace

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_INTERNAL_H_
