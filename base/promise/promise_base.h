// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_BASE_H_
#define BASE_PROMISE_PROMISE_BASE_H_

#include <list>
#include <set>
#include <vector>

#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "base/variant.h"

namespace base {

struct Void {};

template <typename T>
class BASE_EXPORT Promise;

namespace internal {
template <typename T>
class BASE_EXPORT InitialPromise;
}  // namespace internal

// Wrapper around base::Value which disambiguates the Reject type in the case of
// Promise<Value>.
class BASE_EXPORT RejectValue {
 public:
  RejectValue(Value value);

  template <class... Args>
  explicit RejectValue(Args&&... args) : value_(std::forward<Args>(args)...) {}

  RejectValue(RejectValue&& other);

  ~RejectValue();

  RejectValue(const RejectValue&) = delete;
  RejectValue& operator=(const RejectValue&) = delete;

  RejectValue Clone() const;

  constexpr Value& operator*() { return value_; }

  constexpr const Value& operator*() const { return value_; }

  constexpr Value* operator->() { return &value_; }

  constexpr const Value* operator->() const { return &value_; }

 private:
  Value value_;
};

// Used for manually resolving and rejecting a Promise.
template <typename T>
class BASE_EXPORT PromiseResolver {
 public:
  PromiseResolver() {}

  explicit PromiseResolver(scoped_refptr<internal::InitialPromise<T>> promise)
      : promise_(std::move(promise)) {}

  void Resolve(T&& t);

  void Reject(RejectValue err);

  template <typename... Args>
  void Reject(Args... args) {
    promise_->RejectInternal(RejectValue(std::forward<Args>(args)...));
  }

  scoped_refptr<internal::InitialPromise<T>> promise_;
};

template <>
class BASE_EXPORT PromiseResolver<void> {
 public:
  PromiseResolver() {}

  explicit PromiseResolver(
      scoped_refptr<internal::InitialPromise<void>> promise)
      : promise_(std::move(promise)) {}

  void Resolve();

  void Reject(RejectValue err);

  template <typename... Args>
  void Reject(Args... args);

  scoped_refptr<internal::InitialPromise<void>> promise_;
};

namespace internal {
template <typename ReturnT, typename... Promises>
class AllPromise;

class BASE_EXPORT PromiseBase : public RefCounted<PromiseBase> {
 public:
  PromiseBase(const PromiseBase&) = delete;
  PromiseBase& operator=(const PromiseBase&) = delete;

  enum class State {
    UNRESOLVED,
    RESOLVED_WITH_PROMISE,
    RESOLVED,
    REJECTED,
    DEPENDENCY_REJECTED,
    AFTER_FINALLY,
    CANCELLED
  };

  State state() const { return state_; }

  struct ExecutorResult {
    explicit ExecutorResult(State new_state);
    explicit ExecutorResult(scoped_refptr<PromiseBase> curried_promise);
    ~ExecutorResult();

    ExecutorResult(ExecutorResult&&);
    ExecutorResult(const ExecutorResult&) = delete;
    ExecutorResult& operator=(const ExecutorResult&) = delete;

    State new_state;
    scoped_refptr<PromiseBase> curried_promise;
  };

  size_t creation_order() const { return creation_order_; }

  enum class PrerequisitePolicy { ALL, ANY, ALWAYS, NEVER, FINALLY };

  PrerequisitePolicy prerequisite_policy() const {
    return prerequisite_policy_;
  }

  // Prevents this promise from running an executor, and cancels all dependent
  // promises unless they have PrerequisitePolicy::ANY, in which case that will
  // only be canceled if all of it's prerequisites are canceled.
  void Cancel();

  struct PromiseContext;

 protected:
  template <typename T>
  friend class base::Promise;

  template <typename ReturnT, typename... Promises>
  friend class AllPromise;

  PromiseBase();

  PromiseBase(Optional<Location> from_here,
              scoped_refptr<SequencedTaskRunner> task_runner,
              State state,
              PrerequisitePolicy prerequisite_policy,
              std::vector<scoped_refptr<PromiseBase>> prerequisites);

  friend class base::RefCounted<PromiseBase>;

  virtual ~PromiseBase();

  // Checks if this promise is ready to execute.
  bool CanExecute() const;

  // Calls Process() if CanExecute() returns true.
  void ExecuteIfPossible();

  static void Process();

  // Calls |RunExecutor()| or posts a task to do so if |from_here_| is not
  // nullopt.
  void Execute();

  void DetachFromPrerequistes();

  // Runs either the promise's on resolve or on reject executor callback
  // depending on |state_|.
  virtual ExecutorResult RunExecutor();

  // Applies the |resolve_result|.
  void OnExecutorResult(ExecutorResult resolve_result);

  // Applies the |resolve_result| and checks if any dependents can now execute.
  void OnManualResolve(ExecutorResult resolve_result);

  virtual bool HasOnResolveExecutor() = 0;

  virtual bool HasOnRejectExecutor() = 0;

  // Returns the promise's rejection reason.  Only valid to call on a rejected
  // promise.
  virtual const RejectValue* GetRejectValue() const = 0;

  // If the promise doesn't have a rejection handler this is used to copy the
  // rejection reason.
  virtual void PropagateRejectValue(const RejectValue& reject_reason) = 0;

  // Returns the first canceled prerequisite if any or null if there isn't one.
  PromiseBase* GetFirstRejectedPrerequisite();

  // Works out if we should cancel too if a prerequisite was canceled.
  bool ShouldCancelToo() const;

  static size_t next_creation_order_;

  struct Comparator {
    bool operator()(const scoped_refptr<PromiseBase>& op1,
                    const scoped_refptr<PromiseBase>& op2) const;
  };

  const size_t creation_order_;
  const Optional<Location> from_here_;
  const scoped_refptr<SequencedTaskRunner> task_runner_;
  State state_ = State::UNRESOLVED;
  PrerequisitePolicy prerequisite_policy_ = PrerequisitePolicy::ALL;
  std::vector<scoped_refptr<PromiseBase>> prerequisites_;
  std::set<scoped_refptr<PromiseBase>, Comparator> dependants_;
};

template <typename T>
class BASE_EXPORT PromiseT : public PromiseBase {
 public:
  // Neither Variant<> or std::tuple<> can contain void but they can contain the
  // empty type Void.
  using NonVoidT =
      typename std::conditional<std::is_void<T>::value, Void, T>::type;

  using ValueT = Variant<Monostate, NonVoidT, RejectValue>;

  enum {
    kResolvedIndex = 1,  // Index of the resolved type in |ValueT|.
    kRejectedIndex = 2,  // Index of the rejected type in |ValueT|.
  };

  using in_place_resolved_t = in_place_index_t<kResolvedIndex>;
  using in_place_rejected_t = in_place_index_t<kRejectedIndex>;

  virtual Optional<PromiseResolver<T>> GetResolver() { return nullopt; }

  virtual const ValueT& value() const { return value_; };
  virtual ValueT& value() { return value_; };

  PromiseT(NonVoidT resolve)
      : PromiseBase(nullopt,
                    nullptr,
                    State::RESOLVED,
                    PrerequisitePolicy::NEVER,
                    {}),
        value_(in_place_index_t<kResolvedIndex>(), std::move(resolve)) {}

  PromiseT(RejectValue reject)
      : PromiseBase(nullopt,
                    nullptr,
                    State::REJECTED,
                    PrerequisitePolicy::NEVER,
                    {}),
        value_(in_place_index_t<kRejectedIndex>(), std::move(reject)) {}

 protected:
  PromiseT() {}

  PromiseT(Optional<Location> from_here,
           scoped_refptr<SequencedTaskRunner> task_runner,
           PrerequisitePolicy prerequisite_policy,
           std::vector<scoped_refptr<PromiseBase>> prerequisites)
      : PromiseBase(std::move(from_here),
                    std::move(task_runner),
                    PromiseBase::State::UNRESOLVED,
                    prerequisite_policy,
                    std::move(prerequisites)) {}

  ~PromiseT() override {}

  PromiseT(const PromiseT&) = delete;
  PromiseT& operator=(const PromiseT&) = delete;

  const RejectValue* GetRejectValue() const override {
    return &Get<RejectValue>(&value_);
  }

  void PropagateRejectValue(const RejectValue& reject_reason) override {
    value_.emplace(in_place_index_t<kRejectedIndex>(), reject_reason.Clone());
  }

  bool HasOnResolveExecutor() override { return false; }

  bool HasOnRejectExecutor() override { return false; }

  ValueT value_;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_PROMISE_BASE_H_
