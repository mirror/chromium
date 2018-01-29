// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/abstract_promise.h"
#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_task_runner_handle.h"

namespace base {
namespace internal {

size_t AbstractPromise::next_creation_order_ = 0;

AbstractPromise::~AbstractPromise() {}

void AbstractPromise::InsertPrequisites() {
  for (scoped_refptr<AbstractPromise>& prerequisite : prerequisites_) {
    prerequisite->dependants_.insert(this);
  }
}

bool AbstractPromise::CanExecute() const {
  if (state_ != State::UNRESOLVED && state_ != State::RESOLVED_WITH_PROMISE)
    return false;

  switch (prerequisite_policy_) {
    case PrerequisitePolicy::ALL: {
      bool all_resolved = true;
      for (const scoped_refptr<AbstractPromise>& prerequisite :
           prerequisites_) {
        if (prerequisite->state() == State::UNRESOLVED ||
            prerequisite->state() == State::RESOLVED_WITH_PROMISE) {
          all_resolved = false;
        }
        // Run immediately if anything was rejected.
        if (prerequisite->state() == State::REJECTED)
          return true;
      }
      // Otherwise only run if everything was resolved.
      return all_resolved;
    }

    case PrerequisitePolicy::ANY:
      DCHECK(!prerequisites_.empty());
      for (const scoped_refptr<AbstractPromise>& prerequisite :
           prerequisites_) {
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
      for (const scoped_refptr<AbstractPromise>& dependant :
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

void AbstractPromise::Execute() {
  if (state_ == State::RESOLVED_WITH_PROMISE) {
    state_ = Get<scoped_refptr<AbstractPromise>>(&value_)->state_;
    return;
  }

  DCHECK_NE(state_, State::CANCELLED);
  DCHECK(executor_);

  executor_->Execute(this);
  executor_ = nullptr;

  if (state_ != State::CANCELLED)
    SetStateFromValue();
}

void AbstractPromise::SetStateFromValue() {
  DCHECK_NE(state_, State::CANCELLED);

  switch (value_.index()) {
    case 1:
      state_ = State::RESOLVED;
      break;

    case 2:
      state_ = State::REJECTED;
      break;

    case 3: {
      scoped_refptr<AbstractPromise> curried_promise =
          Get<scoped_refptr<AbstractPromise>>(&value_);

      State curried_promise_state = curried_promise->state();
      if (curried_promise_state == State::RESOLVED ||
          curried_promise_state == State::REJECTED) {
        // Curried promise has already settled.
        state_ = curried_promise_state;
        bool success = value_.TryAssign(curried_promise->value());
        DCHECK(success);
      } else {
        state_ = State::RESOLVED_WITH_PROMISE;
        // The promise was partially resolved. We can throw away any
        // pre-existing prerequisites in favor of the new one.
        for (std::vector<scoped_refptr<AbstractPromise>>::iterator iter =
                 prerequisites_.begin();
             iter != prerequisites_.end(); iter++) {
          (*iter)->dependants_.erase(this);
        }
        prerequisites_.clear();

        curried_promise->dependants_.insert(this);
        prerequisites_.push_back(std::move(curried_promise));
        prerequisite_policy_ = PrerequisitePolicy::ALL;
      }
      break;
    }

    default:
      DCHECK(state_ == AbstractPromise::State::AFTER_FINALLY)
          << "Unexpected value index " << value_.index();
      break;
  }
}

struct AbstractPromise::PromiseContext {
  std::list<scoped_refptr<AbstractPromise>> ready_promises;
  bool task_posted = false;
  bool in_process = false;

  void MaybeProcess() {
    if (in_process || ready_promises.empty())
      return;

    if (ready_promises.front()->from_here_) {
      MaybePostTask();
    } else {
      AbstractPromise::Process();
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
    AbstractPromise::Process();
  }
};

static LazyInstance<AbstractPromise::PromiseContext>::Leaky s_promise_context;

void AbstractPromise::OnManualResolve() {
  if (state_ == State::CANCELLED)
    return;

  SetStateFromValue();

  for (auto& dependee : dependants_) {
    if (dependee->CanExecute()) {
      s_promise_context.Get().ready_promises.push_back(dependee);
      // Prevent double insertion.
      dependee->prerequisite_policy_ = PrerequisitePolicy::NEVER;
    }
  }

  s_promise_context.Get().MaybeProcess();
}

void AbstractPromise::ExecuteIfPossible() {
  if (!CanExecute())
    return;

  s_promise_context.Get().ready_promises.push_back(this);

  // Prevent double insertion.
  prerequisite_policy_ = PrerequisitePolicy::NEVER;

  s_promise_context.Get().MaybeProcess();
}

AbstractPromise* AbstractPromise::GetFirstRejectedPrerequisite() {
  for (scoped_refptr<AbstractPromise>& prerequisite : prerequisites_) {
    if (prerequisite->state() == State::REJECTED)
      return prerequisite.get();
  }
  return nullptr;
}

// static
void AbstractPromise::Process() {
  s_promise_context.Get().in_process = true;
  while (!s_promise_context.Get().ready_promises.empty()) {
    scoped_refptr<AbstractPromise> promise =
        std::move(s_promise_context.Get().ready_promises.front());
    s_promise_context.Get().ready_promises.pop_front();

    if (promise->state_ != State::CANCELLED) {
      promise->Execute();

      if (promise->state_ == State::RESOLVED_WITH_PROMISE)
        continue;

      for (auto& dependee : promise->dependants_) {
        if (dependee->CanExecute()) {
          s_promise_context.Get().ready_promises.push_back(dependee);
          // Prevent double insertion.
          dependee->prerequisite_policy_ = PrerequisitePolicy::NEVER;
        }
      }

      // Check if any co-dependent finally promises can now fire.
      for (scoped_refptr<AbstractPromise>& prerequisite :
           promise->prerequisites_) {
        prerequisite->dependants_.erase(promise.get());
        for (auto& co_dependee : prerequisite->dependants_) {
          if (co_dependee->prerequisite_policy_ ==
                  PrerequisitePolicy::FINALLY &&
              co_dependee->CanExecute()) {
            s_promise_context.Get().ready_promises.push_back(co_dependee);
            // Prevent double insertion.
            co_dependee->prerequisite_policy_ = PrerequisitePolicy::NEVER;
          }
        }
      }
      promise->prerequisites_.clear();
    }

    if (s_promise_context.Get().ready_promises.empty())
      break;

    if (s_promise_context.Get().ready_promises.front()->from_here_) {
      s_promise_context.Get().MaybePostTask();
      break;
    }
  }
  s_promise_context.Get().in_process = false;
}

void AbstractPromise::Cancel() {
  state_ = State::CANCELLED;
  prerequisites_.clear();

  // We need to be careful how we traverse the graph in case of cycles (which
  // are unlikely but possible).
  std::set<scoped_refptr<AbstractPromise>, Comparator> dependants;
  std::swap(dependants, dependants_);

  for (const scoped_refptr<AbstractPromise>& dependant : dependants) {
    if (dependant->ShouldCancelToo())
      dependant->Cancel();
  }

  if (state_ == State::CANCELLED)
    return;
}

bool AbstractPromise::ShouldCancelToo() const {
  if (prerequisite_policy_ != PrerequisitePolicy::ANY)
    return true;

  for (const scoped_refptr<AbstractPromise>& dependant : dependants_) {
    if (dependant->state_ != State::CANCELLED)
      return false;
  }
  return true;
}

bool AbstractPromise::IsInitialPromise() const {
  return !executor_ && state_ == State::UNRESOLVED;
}

bool AbstractPromise::Comparator::operator()(
    const scoped_refptr<AbstractPromise>& op1,
    const scoped_refptr<AbstractPromise>& op2) const {
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

}  // namespace internal
}  // namespace base
