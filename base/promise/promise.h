// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_H_
#define BASE_PROMISE_PROMISE_H_

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/promise/promise_forward.h"
#include "base/promise/promise_internal.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace base {

// Creates a pair of Future and Promise.
// The resulting Future and Promise forms a thread-safe one-shot channel from
// the Promise to the Future.
template <typename ResolveT, typename RejectT = void>
std::tuple<Future<ResolveT, RejectT>, Promise<ResolveT, RejectT>> Contract() {
  using Value = internal::PromiseValue<ResolveT, RejectT>;
  using Storage = internal::PromiseStorage<Value>;
  Storage* storage = new Storage;
  return std::make_tuple(Future<ResolveT, RejectT>(storage),
                         Promise<ResolveT, RejectT>(storage));
}

// Promise is a sender of the channel created by Contract() above.
// The instance is safe to pass or be used on any thread.
template <typename ResolveT, typename RejectT>
class Promise {
 private:
  using Value = internal::PromiseValue<ResolveT, RejectT>;
  using Storage = internal::PromiseStorage<Value>;

 public:
  // Promise is a move-only default-constructible class.
  Promise() = default;
  Promise(const Promise&) = delete;
  Promise(Promise&&) = default;
  ~Promise() = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) = default;

  // Notifies the corresponding Future that the Promise is resolved with a
  // given value.
  // This function consumes the Promise instance, and nulls out the instance.
  template <typename... Args>
  void Resolve(const Location& from_here, Args&&... args) && {
    DCHECK(storage_);
    storage_.release()->Settle(from_here, internal::resolved_tag,
                               std::forward<Args>(args)...);
  }

  // Notifies the corresponding Future that the Promise is rejected with a
  // given value.
  // This function consumes the Promise instance, and nulls out the instance.
  template <typename... Args>
  void Reject(const Location& from_here, Args&&... args) && {
    DCHECK(storage_);
    storage_.release()->Settle(from_here, internal::rejected_tag,
                               std::forward<Args>(args)...);
  }

  // Notifies the corresponding Future that the Promise is resolved or rejected
  // depends on the state of |value|. This method is mostly for internal use.
  // This function consumes the Promise instance, and nulls out the instance.
  template <typename ResT, typename RejT>
  void Settle(const Location& from_here,
              internal::PromiseValue<ResT, RejT> value) && {
    DCHECK(storage_);
    storage_.release()->Settle(from_here, std::move(value));
  }

  // After the given |future| is resolved or rejected, notifies the
  // corresponding Future that the Promise is resolved or rejected. This
  // function consumes the Promise instance, and nulls out the instance.
  template <typename ResT, typename RejT>
  void Settle(const Location& from_here, Future<ResT, RejT> future) && {
    DCHECK(storage_);
    auto relay = [](Promise promise, const Location& from_here,
                    internal::PromiseValue<ResT, RejT> value) {
      DCHECK(promise.storage_);
      promise.storage_.release()->Settle(from_here, std::move(value));
    };
    std::move(future).Attach(internal::SynchronousTaskRunner::GetInstance(),
                             BindOnce(relay, std::move(*this), from_here));
  }

 private:
  friend std::tuple<Future<ResolveT, RejectT>, Promise<ResolveT, RejectT>>
  Contract<ResolveT, RejectT>();

  explicit Promise(Storage* storage) : storage_(storage) {}

  struct DefaultOnScopeOut {
    void operator()(Storage* storage) const { storage->OnDefaulted(); }
  };
  std::unique_ptr<Storage, DefaultOnScopeOut> storage_;
};

// Future is a receiver of the channel created by Contract() above.
// The instance is safe to pass or be used on any thread.
template <typename ResolveT, typename RejectT>
class Future {
 private:
  using Value = internal::PromiseValue<ResolveT, RejectT>;
  using Storage = internal::PromiseStorage<Value>;

  template <typename R>
  using ResolveSignature =
      typename internal::HandlerTypeHelper<ResolveT>::template Type<R>;

  template <typename R>
  using RejectSignature =
      typename internal::HandlerTypeHelper<RejectT>::template Type<R>;

 public:
  // Future is a move-only default-constructible class.
  Future() = default;
  Future(const Future&) = delete;
  Future(Future&&) = default;
  ~Future() = default;
  Future& operator=(const Future&) = delete;
  Future& operator=(Future&&) = default;

  // A Then() variant that uses the current sequence to run the handler.
  template <typename R>
  typename internal::ThenCorruptionHelper<R, RejectT>::ResultType Then(
      const Location& from_here,
      OnceCallback<ResolveSignature<R>> handler) && {
    return std::move(*this).Then(from_here, SequencedTaskRunnerHandle::Get(),
                                 std::move(handler));
  }

  // Sets |handler| that handles the Promise resolution. This is safe to call on
  // any thread. Returns a Future that wraps the return value of |handler|. If
  // |handler| returned a Future, Then() flattens it to non-nested Future.
  // Example:
  //   Future<int, std::string> future;
  //
  //   // Handles an int as the value of Promise, and returns a double value.
  //   Future<double, std::string> future2 =
  //       std::move(future).Then(FROM_HERE, BindOnce([](int x) {
  //           return 1.0;
  //       }));  // Returns Future<double, std::string> for the Future chaining.
  //
  //   std::move(future2).Then(FROM_HERE, BindOnce([](double y) {
  //      // Return a Future for resolving the chained Future.
  //      return Resolve(FROM_HERE, 'x');
  //   }));  // Returns Future<char, std::string>, instead of
  //         // Future<Future<char, NotReached>, std::string>.
  template <typename R>
  typename internal::ThenCorruptionHelper<R, RejectT>::ResultType Then(
      const Location& from_here,
      scoped_refptr<TaskRunner> task_runner,
      OnceCallback<ResolveSignature<R>> handler) && {
    DCHECK(storage_);
    using Helper = internal::ThenCorruptionHelper<R, RejectT>;
    using ResolveType = typename Helper::ResolveType;
    using RejectType = typename Helper::RejectType;
    using FutureType = Future<ResolveType, RejectType>;
    using PromiseType = Promise<ResolveType, RejectType>;

    auto relay = [](const Location& from_here, PromiseType promise,
                    OnceCallback<ResolveSignature<R>> handler, Value value) {
      if (!value.is_resolved_)
        std::move(promise).Reject(from_here, std::move(value.rejected_));

      Helper::Settle(
          from_here, std::move(promise),
          internal::Apply(std::move(handler), std::move(value.resolved_)));
    };

    FutureType future;
    PromiseType promise;
    std::tie(future, promise) = Contract<ResolveType, RejectType>();
    storage_.release()->Attach(
        std::move(task_runner),
        BindOnce(relay, from_here, std::move(promise), std::move(handler)));
    return future;
  }

  // RepeatingCallback version of Then() above.
  template <typename R>
  typename internal::ThenCorruptionHelper<R, RejectT>::ResultType Then(
      const Location& from_here,
      RepeatingCallback<ResolveSignature<R>> handler) && {
    return std::move(*this).Then(
        from_here, OnceCallback<ResolveSignature<R>>(std::move(handler)));
  }

  // RepeatingCallback version of Then() with an explicit |task_runner|.
  template <typename R>
  typename internal::ThenCorruptionHelper<R, RejectT>::ResultType Then(
      const Location& from_here,
      scoped_refptr<TaskRunner> task_runner,
      RepeatingCallback<ResolveSignature<R>> handler) && {
    return std::move(*this).Then(
        from_here, std::move(task_runner),
        OnceCallback<ResolveSignature<R>>(std::move(handler)));
  }

  // A Catch() variant that uses the current sequence to run the handler.
  template <typename R>
  typename internal::CatchCorruptionHelper<R, ResolveT>::ResultType Catch(
      const Location& from_here,
      OnceCallback<RejectSignature<R>> handler) && {
    return (*this).Catch(from_here, SequencedTaskRunnerHandle::Get(),
                         std::move(handler));
  }

  template <typename R>
  typename internal::CatchCorruptionHelper<R, ResolveT>::ResultType Catch(
      const Location& from_here,
      scoped_refptr<TaskRunner> task_runner,
      OnceCallback<RejectSignature<R>> handler) && {
    DCHECK(storage_);
    using Helper = internal::CatchCorruptionHelper<R, ResolveT>;
    using ResolveType = typename Helper::ResolveType;
    using RejectType = typename Helper::RejectType;
    using FutureType = Future<ResolveType, RejectType>;
    using PromiseType = Promise<ResolveType, RejectType>;

    auto relay = [](const Location& from_here, PromiseType promise,
                    OnceCallback<RejectSignature<R>> handler, Value value) {
      if (value.is_resolved_)
        promise.Resolve(from_here, std::move(value.resolved_));
      Helper::Settle(
          from_here, std::move(promise),
          internal::Apply(std::move(handler), std::move(value.rejected_)));
    };

    FutureType future;
    PromiseType promise;
    std::tie(future, promise) = Contract<ResolveType, RejectType>();
    storage_.release()->Attach(
        std::move(task_runner),
        BindOnce(relay, from_here, std::move(promise), std::move(handler)));
    return future;
  }

  // RepeatingCallback version of Catch() above.
  template <typename R>
  typename internal::CatchCorruptionHelper<R, ResolveT>::ResultType Catch(
      const Location& from_here,
      RepeatingCallback<RejectSignature<R>> handler) && {
    return (*this).Catch(from_here,
                         OnceCallback<RejectSignature<R>>(std::move(handler)));
  }

  // RepeatingCallback version of Catch() above with a explicit |task_runner|.
  template <typename R>
  typename internal::CatchCorruptionHelper<R, ResolveT>::ResultType Catch(
      const Location& from_here,
      scoped_refptr<TaskRunner> task_runner,
      RepeatingCallback<RejectSignature<R>> handler) && {
    return (*this).Catch(from_here, std::move(task_runner),
                         OnceCallback<RejectSignature<R>>(std::move(handler)));
  }

  // Sets a Callback that handles both resolved and rejected cases for internal
  // use.
  void Attach(scoped_refptr<TaskRunner> task_runner,
              OnceCallback<void(Value)> handler) && {
    DCHECK(storage_);
    storage_.release()->Attach(std::move(task_runner), std::move(handler));
  }

 private:
  friend std::tuple<Future<ResolveT, RejectT>, Promise<ResolveT, RejectT>>
  Contract<ResolveT, RejectT>();
  explicit Future(Storage* storage) : storage_(storage) {}

  struct AbandonOnScopeOut {
    void operator()(Storage* storage) const { storage->OnAbandoned(); }
  };
  std::unique_ptr<Storage, AbandonOnScopeOut> storage_;
};

// Returns a Future that is already resolved with void.
inline Future<void, NotReached> Resolve(const Location& from_here) {
  Future<void, NotReached> future;
  Promise<void, NotReached> promise;
  std::tie(future, promise) = Contract<void, NotReached>();
  std::move(promise).Resolve(from_here);
  return future;
}

// Returns a Future that is already resolved with |value|.
template <typename ResolveT>
Future<ResolveT, NotReached> Resolve(const Location& from_here,
                                     ResolveT value) {
  Future<ResolveT, NotReached> future;
  Promise<ResolveT, NotReached> promise;
  std::tie(future, promise) = Contract<ResolveT, NotReached>();
  std::move(promise).Resolve(from_here, std::move(value));
  return future;
}

// Returns a Future that is already rejected with void.
inline Future<NotReached, void> Reject(const Location& from_here) {
  Future<NotReached, void> future;
  Promise<NotReached, void> promise;
  std::tie(future, promise) = Contract<NotReached, NotReached>();
  std::move(promise).Reject(from_here);
  return future;
}

// Returns a Future that is already rejected with |value|.
template <typename RejectT>
Future<NotReached, RejectT> Reject(const Location& from_here, RejectT value) {
  Future<NotReached, RejectT> future;
  Promise<NotReached, RejectT> promise;
  std::tie(future, promise) = Contract<NotReached, RejectT>();
  std::move(promise).Reject(from_here, std::move(value));
  return future;
}

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_H_
