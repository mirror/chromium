// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_INTERNAL_H_
#define BASE_PROMISE_PROMISE_INTERNAL_H_

#include <atomic>
#include <type_traits>
#include <utility>

#include "base/base_export.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/task_runner.h"

namespace base {

template <typename ResolveT, typename RejectT = void>
class Promise;

template <typename ResolveT, typename RejectT = void>
class Future;

// A dummy class that is never instantiated.
struct NotReached {
  NotReached() = delete;
  NotReached(const NotReached&) = delete;
  NotReached(NotReached&&) = delete;
  NotReached& operator=(const NotReached&) = delete;
  NotReached& operator=(NotReached&&) = delete;

  // Non-existing instance of NotReached can be converted to anything, as a
  // contradiction implies anything, so that std::common_type_t<NotReached, T>
  // returns T.
  template <typename T>
  operator T() {
    NOTREACHED();
  }
};

namespace internal {

// An fake TaskRunner that runs the task immediately.
class BASE_EXPORT SynchronousTaskRunner : public TaskRunner {
 public:
  static const scoped_refptr<SynchronousTaskRunner>& GetInstance();

  bool RunsTasksInCurrentSequence() const override;
  bool PostDelayedTask(const Location& from_here,
                       OnceClosure task,
                       base::TimeDelta delay) override;

 private:
  struct Holder;
  SynchronousTaskRunner();
  ~SynchronousTaskRunner() override;

  DISALLOW_COPY_AND_ASSIGN(SynchronousTaskRunner);
};

// A placeholder for handling void on a parameter.
struct Empty {};

// Returns Empty if T is void, otherwise returns T itself.
template <typename T>
using EmptyIfVoid =
    std::conditional_t<std::is_void<std::decay_t<T>>::value, Empty, T>;

// Returns void if T is Empty, otherwise returns T itself.
template <typename T>
using VoidIfEmpty =
    std::conditional_t<std::is_same<std::decay_t<T>, Empty>::value, void, T>;

// Apply() calls the given callback with the given.
// The following overloads translates Empty on the argument or the return value
// from/to void.
template <typename R, typename X, typename Y>
R Apply(OnceCallback<R(X)> cb, Y&& y) {
  return std::move(cb).Run(std::forward<Y>(y));
}

inline Empty Apply(OnceCallback<void()> cb, Empty) {
  std::move(cb).Run();
  return Empty();
}

template <typename R>
R Apply(OnceCallback<R()> cb, Empty) {
  return std::move(cb).Run();
}

template <typename X, typename Y>
Empty Apply(OnceCallback<void(X)> cb, Y&& y) {
  std::move(cb).Run(std::forward<Y>(y));
  return Empty();
}

struct resolved_tag_t {
} resolved_tag;
struct rejected_tag_t {
} rejected_tag;

// Creates an instance of T. Following overloads suppresses the compilation
// failure on NotReached construction on the unreachable paths.
template <typename T, typename... Args>
void CallConstructor(T* obj, Args&&... args) {
  new (obj) T(std::forward<Args>(args)...);
}

inline void CallConstructor(NotReached*, NotReached&&) {
  NOTREACHED();
}

template <typename... Args>
void CallConstructor(NotReached*, Args&&...) {
  NOTREACHED();
}

template <typename T>
void CallConstructor(T*, NotReached&&) {
  NOTREACHED();
}

// Calls the destruction of T.
template <typename T, typename... Args>
void CallDestructor(T* obj) {
  obj->~T();
}

// A tagged union that holds either ResolveT or RejectT. ResolveT and RejectT
// may be void.
template <typename ResolveT, typename RejectT>
class PromiseValue {
 public:
  // Constructs |resolved_|.
  template <typename... Args>
  PromiseValue(resolved_tag_t, Args&&... args) : is_resolved_(true) {
    CallConstructor(&resolved_, std::forward<Args>(args)...);
  }

  // Constructs |rejected_|.
  template <typename... Args>
  PromiseValue(rejected_tag_t, Args&&... args) : is_resolved_(false) {
    CallConstructor(&rejected_, std::forward<Args>(args)...);
  }

  // Transform-move ctor.
  template <typename Res, typename Rej>
  PromiseValue(PromiseValue<Res, Rej>&& other)
      : is_resolved_(other.is_resolved_) {
    if (is_resolved_)
      CallConstructor(&resolved_, std::move(other.resolved_));
    else
      CallConstructor(&rejected_, std::move(other.rejected_));
  }

  ~PromiseValue() {
    if (is_resolved_)
      CallDestructor(&resolved_);
    else
      CallDestructor(&rejected_);
  }

  // true if |resolved_| is valid, else, |rejected_| is valid.
  bool is_resolved_;
  union {
    EmptyIfVoid<ResolveT> resolved_;
    EmptyIfVoid<RejectT> rejected_;
  };
};

// A storage class that is held by both Future and Promise.
// Either Settle() or OnAbandoned() is called exactly once per the instance
// from any thread, and either Attach() or OnAbandoned() is called exactly
// once from any thread.
// The lifetime of PromiseStorage is managed by itself. The instance is
// destroyed when both of connected Future and Promise are consumed.
template <typename Value>
class PromiseStorage {
 public:
  // PromiseStorage is default constructible, non-copyable and non-movable.
  PromiseStorage() = default;
  PromiseStorage(const PromiseStorage&) = delete;
  PromiseStorage(PromiseStorage&&) = delete;
  PromiseStorage& operator=(const PromiseStorage&) = delete;
  PromiseStorage& operator=(PromiseStorage&&) = delete;

  // Resolves or rejects the promise.
  template <typename... Args>
  void Settle(const Location& from_here, Args&&... args) {
    DCHECK(!value_);
    from_here_ = from_here;
    value_.emplace(std::forward<Args>(args)...);
    Proceed();
  }

  // Sets |handler| that handles settled value.
  void Attach(scoped_refptr<TaskRunner> task_runner,
              OnceCallback<void(Value)> handler) {
    DCHECK(task_runner);
    DCHECK(handler);
    DCHECK(!task_runner_);
    DCHECK(!handler_);
    task_runner_ = std::move(task_runner);
    handler_ = std::move(handler);
    Proceed();
  }

  // Called when the Promise is destroyed without being settled.
  void OnDefaulted() {
    DCHECK(!value_);
    Proceed();
  }

  // Called when the Future is destroyed without being attached to a handler.
  void OnAbandoned() {
    DCHECK(!handler_);
    Proceed();
  }

 private:
  // PromiseStorage manages its lifetime by itself.
  ~PromiseStorage() = default;

  // Proceed() is called exact 2 times in a PromiseStorage lifetime.
  void Proceed() {
    if (--progress_)
      return;

    if (!value_) {
      // OnDefaulted() case. The Promise object is destroyed without being
      // settled.
      // TODO(tzik): Consider supporting |on_defaulted| callback handles
      // unresolved promises.
      delete this;
      return;
    }

    if (!handler_) {
      delete this;
      return;
    }

    DCHECK(task_runner_);
    task_runner_->PostTask(from_here_,
                           BindOnce(std::move(handler_), std::move(*value_)));
    delete this;
  }

  std::atomic<int> progress_ = {2};

  scoped_refptr<TaskRunner> task_runner_;
  OnceCallback<void(Value)> handler_;
  Location from_here_;
  Optional<Value> value_;
};

// A helper struct that makes a function type whose parameter is |X|, where
// |X| may be void.
template <typename X>
struct HandlerTypeHelper {
  template <typename R>
  using Type = R(X);
};

template <>
struct HandlerTypeHelper<void> {
  template <typename R>
  using Type = R();
};

// Returns a common type that both |X| and |Y| can be transformed to.
// Both |X| and |Y| may be void or NotReached.
// Note that MergeTypes<NotReached, T> returns T.
template <typename X, typename Y>
using MergeTypes =
    VoidIfEmpty<std::common_type_t<EmptyIfVoid<X>, EmptyIfVoid<Y>>>;

// Constructs the return type of Future::Then().
// If the resulting type of the handler is a non-Future type |R|, this returns
// a Future type that |R| is set to its ResolveT.
// If the resulting type is a Future type, this flatten the nested Future.
// On the Futured-returning handlers, the types of RejectT need to be
// compatible.
template <typename R, typename RejT>
struct ThenCorruptionHelper {
  using ResolveType = R;
  using RejectType = RejT;
  using ResultType = Future<ResolveType, RejectType>;

  static void Settle(const Location& from_here,
                     Promise<ResolveType, RejectType> promise,
                     EmptyIfVoid<ResolveType> value) {
    DCHECK(promise);
    std::move(promise).Settle(from_here, PromiseValue<ResolveType, RejectType>(
                                             resolved_tag, std::move(value)));
  }
};

template <typename NewResT, typename NewRejT, typename RejT>
struct ThenCorruptionHelper<Future<NewResT, NewRejT>, RejT> {
  using ResolveType = NewResT;
  using RejectType = MergeTypes<NewRejT, RejT>;
  using ResultType = Future<ResolveType, RejectType>;

  static void Settle(const Location& from_here,
                     Promise<ResolveType, RejectType> promise,
                     Future<NewResT, NewRejT> future) {
    DCHECK(promise);
    DCHECK(future);
    std::move(promise).Settle(from_here, std::move(future));
  }
};

// Constructs the return type of Future::Catch().
// If the resulting type of the handler is a non-Future type |R|, this returns
// a Future type that |R| is set to its ResolveT.
// If the resulting type is a Future type, this flatten the nested Future.
// On the Futured-returning handlers, the types of ResultT need to be
// compatible.
template <typename R, typename ResT>
struct CatchCorruptionHelper {
  using ResolveType = R;
  using RejectType = NotReached;
  using ResultType = Future<ResolveType, RejectType>;

  static void Settle(const Location& from_here,
                     Promise<ResolveType, RejectType> promise,
                     EmptyIfVoid<ResolveType> value) {
    DCHECK(promise);
    std::move(promise).Settle(from_here, PromiseValue<ResolveType, RejectType>(
                                             resolved_tag, std::move(value)));
  }
};

template <typename NewResT, typename NewRejT, typename ResT>
struct CatchCorruptionHelper<Future<NewResT, NewRejT>, ResT> {
  using ResolveType = MergeTypes<NewResT, ResT>;
  using RejectType = NewRejT;
  using ResultType = Future<ResolveType, RejectType>;

  static void Settle(const Location& from_here,
                     Promise<ResolveType, RejectType> promise,
                     Future<NewResT, NewRejT> future) {
    DCHECK(promise);
    DCHECK(future);
    std::move(promise).Settle(from_here, std::move(future));
  }
};

}  // namespace internal
}  // namespace base

#endif  // BASE_PROMISE_PROMISE_INTERNAL_H_
