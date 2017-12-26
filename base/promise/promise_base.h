// Copyright 2017 The Chromium Authors. All rights reserved.
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
#include "base/values.h"

namespace base {

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

 protected:
  template <typename T>
  friend class Promise;

  PromiseBase();

  PromiseBase(Optional<Location> from_here,
              State state,
              PrerequisitePolicy prerequisite_policy,
              std::vector<scoped_refptr<PromiseBase>> prerequisites);

  friend class base::RefCounted<PromiseBase>;

  virtual ~PromiseBase();

  // Checks if this promise is ready to execute.
  bool CanExecute() const;

  // Calls Process() if CanExecute() returns true.
  void ExecuteIfPossible();

  // Processes the |ready_promises|.
  static void Process(std::list<scoped_refptr<PromiseBase>> ready_promises);

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
  virtual const Value* GetRejectValue() const = 0;

  // If the promise doesn't have a rejection handler this is used to copy the
  // rejection reason.
  virtual void PropagateRejectValue(const Value& reject_reason) = 0;

  // Returns the first canceled prerequisite if any or null if there isn't one.
  PromiseBase* GetCancelledPrerequisite();

  static size_t next_creation_order_;

  struct Comparator {
    bool operator()(const scoped_refptr<PromiseBase>& op1,
                    const scoped_refptr<PromiseBase>& op2);
  };

  const size_t creation_order_;
  const Optional<Location> from_here_;
  State state_ = State::UNRESOLVED;
  PrerequisitePolicy prerequisite_policy_ = PrerequisitePolicy::ALL;
  std::vector<scoped_refptr<PromiseBase>> prerequisites_;
  std::set<scoped_refptr<PromiseBase>, Comparator> dependants_;
};

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_BASE_H_
