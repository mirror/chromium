// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/promise/promise.h"
#include "base/threading/thread_task_runner_handle.h"

namespace base {

RejectValue::RejectValue(Value value) : value_(std::move(value)) {}

RejectValue::RejectValue(RejectValue&& other) = default;

RejectValue::~RejectValue() {}

RejectValue RejectValue::Clone() const {
  return RejectValue(value_.Clone());
}

size_t PromiseBase::next_creation_order_ = 0;

PromiseBase::PromiseBase()
    : creation_order_(next_creation_order_++), from_here_(nullopt) {}

PromiseBase::~PromiseBase() {}

PromiseBase::PromiseBase(Optional<Location> from_here,
                         scoped_refptr<SequencedTaskRunner> task_runner,
                         State state,
                         PrerequisitePolicy prerequisite_policy,
                         std::vector<scoped_refptr<PromiseBase>> prerequisites)
    : creation_order_(next_creation_order_++),
      from_here_(std::move(from_here)),
      task_runner_(std::move(task_runner)),
      state_(state),
      prerequisite_policy_(prerequisite_policy),
      prerequisites_(std::move(prerequisites)) {
  for (scoped_refptr<PromiseBase>& prerequisite : prerequisites_) {
    prerequisite->dependants_.insert(this);
    if (prerequisite->state_ == State::DEPENDENCY_REJECTED ||
        prerequisite->state_ == State::REJECTED) {
      state_ = State::DEPENDENCY_REJECTED;
    }
  }
}

bool PromiseBase::CanExecute() const {
  if (state_ == State::RESOLVED || state_ == State::REJECTED)
    return false;

  switch (prerequisite_policy_) {
    case PrerequisitePolicy::ALL:
      if (state_ == State::DEPENDENCY_REJECTED) {
        return true;
      }

      for (const scoped_refptr<PromiseBase>& prerequisite : prerequisites_) {
        if (prerequisite->state() == State::UNRESOLVED ||
            prerequisite->state() == State::RESOLVED_WITH_PROMISE ||
            prerequisite->state() == State::DEPENDENCY_REJECTED) {
          return false;
        }
      }
      return true;

    case PrerequisitePolicy::ANY:
      DCHECK(!prerequisites_.empty());
      if (state_ == State::DEPENDENCY_REJECTED) {
        return true;
      }

      for (const scoped_refptr<PromiseBase>& prerequisite : prerequisites_) {
        if (prerequisite->state() == State::RESOLVED ||
            prerequisite->state() == State::REJECTED) {
          return true;
        }
      }
      return false;

    case PrerequisitePolicy::FINALLY:
      DCHECK_EQ(prerequisites_.size(), 1u);
      if (prerequisites_[0]->state() != State::RESOLVED &&
          prerequisites_[0]->state() != State::REJECTED) {
        return false;
      }
      // Finally promises are only eligible to run if the prerequisite's
      // dependents are all PrerequisitePolicy::FINALLY.
      for (const scoped_refptr<PromiseBase>& dependant :
           prerequisites_[0]->dependants_) {
        if (dependant->prerequisite_policy_ != PrerequisitePolicy::FINALLY) {
          return false;
        }
      }
      return true;

    case PrerequisitePolicy::ALWAYS:
      return true;

    case PrerequisitePolicy::NEVER:
      return false;
  }

  return false;
}

PromiseBase::ExecutorResult PromiseBase::RunExecutor() {
  NOTREACHED();
  return ExecutorResult(State::REJECTED);
}

void PromiseBase::OnExecutorResult(ExecutorResult resolve_result) {
  State new_state = resolve_result.new_state;

  if (state_ == new_state)
    return;

  if (resolve_result.new_state == State::RESOLVED_WITH_PROMISE) {
    State curried_promise_state = resolve_result.curried_promise->state();
    if (curried_promise_state == State::RESOLVED ||
        curried_promise_state == State::REJECTED) {
      // Curried promise has already settled.
      new_state = curried_promise_state;
    }
    // The promise was partially resolved. We can throw away any pre-existing
    // prerequisites in favor of the new one.
    for (std::vector<scoped_refptr<PromiseBase>>::iterator iter =
             prerequisites_.begin();
         iter != prerequisites_.end(); iter++) {
      (*iter)->dependants_.erase(this);
    }
    prerequisites_.clear();

    resolve_result.curried_promise->dependants_.insert(this);
    prerequisites_.push_back(std::move(resolve_result.curried_promise));
    prerequisite_policy_ = PrerequisitePolicy::ALL;
  }

  state_ = new_state;
}

struct PromiseBase::PromiseContext {
  std::list<scoped_refptr<PromiseBase>> ready_promises;
  bool task_posted = false;
  bool in_process = false;

  void MaybeProcess() {
    if (in_process || ready_promises.empty())
      return;

    if (ready_promises.front()->from_here_) {
      MaybePostTask();
    } else {
      PromiseBase::Process();
    }
  }

  void MaybePostTask() {
    if (task_posted)
      return;

    task_posted = true;
    if (ready_promises.front()->task_runner_) {
      ready_promises.front()->task_runner_->PostTask(
          *ready_promises.front()->from_here_,
          Bind(&PromiseContext::ProcessTask, Unretained(this)));
    } else {
      ThreadTaskRunnerHandle::Get()->PostTask(
          *ready_promises.front()->from_here_,
          Bind(&PromiseContext::ProcessTask, Unretained(this)));
    }
  }

  void ProcessTask() {
    task_posted = false;
    PromiseBase::Process();
  }
};

static LazyInstance<PromiseBase::PromiseContext>::Leaky s_promise_context;

void PromiseBase::OnManualResolve(ExecutorResult resolve_result) {
  OnExecutorResult(std::move(resolve_result));

  for (auto& dependee : dependants_) {
    if (state_ == State::REJECTED)
      dependee->state_ = State::DEPENDENCY_REJECTED;
    if (dependee->CanExecute()) {
      s_promise_context.Get().ready_promises.push_back(dependee);
      // Prevent double insertion.
      dependee->prerequisite_policy_ = PrerequisitePolicy::NEVER;
    }
  }

  s_promise_context.Get().MaybeProcess();
}

void PromiseBase::ExecuteIfPossible() {
  if (!CanExecute())
    return;

  s_promise_context.Get().ready_promises.push_back(this);

  // Prevent double insertion.
  prerequisite_policy_ = PrerequisitePolicy::NEVER;

  s_promise_context.Get().MaybeProcess();
}

PromiseBase* PromiseBase::GetCancelledPrerequisite() {
  for (scoped_refptr<PromiseBase>& prerequisite : prerequisites_) {
    if (prerequisite->state() == State::REJECTED)
      return prerequisite.get();
  }
  return nullptr;
}

// static
void PromiseBase::Process() {
  s_promise_context.Get().in_process = true;
  while (!s_promise_context.Get().ready_promises.empty()) {
    scoped_refptr<PromiseBase> promise =
        std::move(s_promise_context.Get().ready_promises.front());
    s_promise_context.Get().ready_promises.pop_front();
    // A Then or a Catch promise might not have a specific handler, in that
    // case just propagate the state.
    if (promise->state_ == State::DEPENDENCY_REJECTED &&
        !promise->HasOnRejectExecutor()) {
      DCHECK(promise->GetCancelledPrerequisite());
      promise->PropagateRejectValue(
          *promise->GetCancelledPrerequisite()->GetRejectValue());
      promise->state_ = State::REJECTED;
    } else if (promise->state_ != State::DEPENDENCY_REJECTED &&
               !promise->HasOnResolveExecutor()) {
      // Ideally we'd propagate the resolved value like ES6 promises do but
      // this is rather tricky to do in C++ because the promise template args
      // might not match and std::any (when available) is quite ugly to use.
      promise->state_ = State::RESOLVED;
    } else {
      promise->Execute();
    }

    if (promise->state_ == State::RESOLVED_WITH_PROMISE)
      continue;

    for (auto& dependee : promise->dependants_) {
      if (promise->state_ == State::REJECTED)
        dependee->state_ = State::DEPENDENCY_REJECTED;
      if (dependee->CanExecute()) {
        s_promise_context.Get().ready_promises.push_back(dependee);
        // Prevent double insertion.
        dependee->prerequisite_policy_ = PrerequisitePolicy::NEVER;
      }
    }

    // Check if any co-dependent finally promises can now fire.
    for (scoped_refptr<PromiseBase>& prerequisite : promise->prerequisites_) {
      prerequisite->dependants_.erase(promise.get());
      for (auto& co_dependee : prerequisite->dependants_) {
        if (co_dependee->prerequisite_policy_ == PrerequisitePolicy::FINALLY &&
            co_dependee->CanExecute()) {
          s_promise_context.Get().ready_promises.push_back(co_dependee);
          // Prevent double insertion.
          co_dependee->prerequisite_policy_ = PrerequisitePolicy::NEVER;
        }
      }
    }

    promise->prerequisites_.clear();

    if (s_promise_context.Get().ready_promises.empty())
      break;

    if (s_promise_context.Get().ready_promises.front()->from_here_) {
      s_promise_context.Get().MaybePostTask();
      break;
    }
  }
  s_promise_context.Get().in_process = false;
}

void PromiseBase::Execute() {
  if (state_ == State::RESOLVED_WITH_PROMISE) {
    OnExecutorResult(ExecutorResult(State::RESOLVED));
  } else {
    OnExecutorResult(RunExecutor());
  }
}

bool PromiseBase::Comparator::operator()(
    const scoped_refptr<PromiseBase>& op1,
    const scoped_refptr<PromiseBase>& op2) const {
  // Finally promises need to go last.
  if (op1->prerequisite_policy() == PrerequisitePolicy::FINALLY) {
    if (op2->prerequisite_policy() == PrerequisitePolicy::FINALLY) {
      return op1->creation_order() < op2->creation_order();
    } else {
      return false;
    }
  } else {
    if (op2->prerequisite_policy() == PrerequisitePolicy::FINALLY) {
      return true;
    } else {
      return op1->creation_order() < op2->creation_order();
    }
  }
}

PromiseBase::ExecutorResult::ExecutorResult(State new_state)
    : new_state(new_state) {}

PromiseBase::ExecutorResult::ExecutorResult(
    scoped_refptr<PromiseBase> curried_promise)
    : new_state(State::RESOLVED_WITH_PROMISE),
      curried_promise(std::move(curried_promise)) {}

PromiseBase::ExecutorResult::~ExecutorResult() {}

PromiseBase::ExecutorResult::ExecutorResult(ExecutorResult&& other) = default;

}  // namespace base
