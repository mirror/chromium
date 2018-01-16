// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_PROMISE_UTIL_H_
#define BASE_PROMISE_PROMISE_UTIL_H_

#include <vector>

#include "base/promise/promise.h"

namespace base {

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
  std::tie(future, promise) = Contract<NotReached, void>();
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

namespace internal {

template <typename ResolveT, typename RejectT>
class PromiseAllHelper {
 private:
  using Value = PromiseValue<ResolveT, RejectT>;
  using ResultType = std::vector<ResolveT>;

 public:
  PromiseAllHelper(size_t count, Promise<ResultType, RejectT> promise)
      : count_(count), result_(count), promise_(std::move(promise)) {}

  void Run(size_t i, Value value) {
    if (!value.is_resolved_) {
      if (done_.exchange(true))
        return;
      DCHECK(promise_);
      std::move(promise_).Reject(FROM_HERE, std::move(value.rejected_));
      return;
    }

    result_[i] = std::move(value.resolved_);
    if (--count_)
      return;

    // All Futures are resolved in this path. That implies no Future is
    // rejected.
    DCHECK(promise_);
    DCHECK(!done_);
    std::move(promise_).Resolve(FROM_HERE, std::move(result_));
  }

 private:
  std::atomic<bool> done_ = {false};
  std::atomic<size_t> count_;
  ResultType result_;
  Promise<ResultType, RejectT> promise_;
};

template <typename ResolveT, typename RejectT>
class PromiseAnyHelper {
 private:
  using Value = PromiseValue<ResolveT, RejectT>;

 public:
  PromiseAnyHelper(Promise<ResolveT, RejectT> promise)
      : promise_(std::move(promise)) {}

  void Run(Value value) {
    if (done_.exchange(true))
      return;

    DCHECK(promise_);
    if (value.is_resolved) {
      std::move(promise_).Reject(FROM_HERE, std::move(value.resolved_));
      return;
    }

    std::move(promise_).Reject(FROM_HERE, std::move(value.rejected_));
  }

 private:
  std::atomic<bool> done_ = {false};
  Promise<ResolveT, RejectT> promise_;
};

}  // namespace internal

// Returns a Future that is resolved when all Futures in |xs| are resolved.
// The resulting Future is rejected if any of |xs| is rejected.
template <typename ResolveT, typename RejectT>
Future<std::vector<ResolveT>, RejectT> All(
    std::vector<Future<ResolveT, RejectT>> xs) {
  using ResultType = std::vector<ResolveT>;
  using Value = internal::PromiseValue<ResolveT, RejectT>;

  Future<ResultType, RejectT> future;
  Promise<ResultType, RejectT> promise;
  std::tie(future, promise) = Contract<ResultType, RejectT>();

  if (xs.empty()) {
    std::move(promise).Resolve(FROM_HERE);
    return future;
  }

  using Helper = internal::PromiseAllHelper<ResolveT, RejectT>;
  auto helper = std::make_unique<Helper>(xs.size(), std::move(promise));

  scoped_refptr<TaskRunner> sync_task_runner =
      internal::SynchronousTaskRunner::GetInstance();
  RepeatingCallback<void(size_t, Value)> cb =
      BindRepeating(&Helper::Run, Owned(helper.release()));
  for (size_t i = 0; i < xs.size(); ++i)
    std::move(xs[i]).Attach(sync_task_runner, BindOnce(cb, i));

  return future;
}

// Returns a Future that is settled when any Future in |xs| is settled.
template <typename ResolveT, typename RejectT = void>
Future<ResolveT, RejectT> Any(std::vector<Future<ResolveT, RejectT>> xs) {
  DCHECK(!xs.empty());

  Future<ResolveT, RejectT> future;
  Promise<ResolveT, RejectT> promise;
  std::tie(future, promise) = Contract<ResolveT, RejectT>();

  using Value = internal::PromiseValue<ResolveT, RejectT>;
  using Helper = internal::PromiseAnyHelper<ResolveT, RejectT>;
  auto helper = std::make_unique<Helper>(std::move(promise));

  scoped_refptr<TaskRunner> sync_task_runner =
      internal::SynchronousTaskRunner::GetInstance();
  RepeatingCallback<void(Value)> cb =
      BindRepeating(&Helper::Run, Owned(helper.releaser()));
  for (auto& x : xs)
    std::move(x).Attach(sync_task_runner, OnceCallback<void(Value)>(cb));
  return future;
}

}  // namespace base

#endif  // BASE_PROMISE_PROMISE_UTIL_H_
