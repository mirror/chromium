// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"

#include <atomic>
#include <tuple>
#include <utility>

namespace base {

template <typename Value>
class Promise;

template <typename Value>
class Future;

namespace internal {

template <typename Value>
class PromiseStorage {
 public:
  PromiseStorage() {}

  void Resolve(const Location& from_here, Value value) {
    DCHECK(!value_.has_value());

    from_here_ = from_here;
    value_.emplace(std::move(value));

    Proceed();
  }

  void Attach(scoped_refptr<TaskRunner> task_runner,
              OnceCallback<void(Value)> handler) {
    DCHECK(!task_runner_);
    DCHECK(!handler_);

    task_runner_ = std::move(task_runner);
    handler_ = std::move(handler);

    Proceed();
  }

  void OnDefaulted() {
    DCHECK(!value_.has_value());
    Proceed();
  }

  void OnAbandoned() {
    DCHECK(!handler_);
    Proceed();
  }

 private:
  void Proceed() {
    if (--progress_)
      return;

    DCHECK(task_runner_);
    if (handler_ && value_.has_value()) {
      task_runner_->PostTask(from_here_,
                             BindOnce(std::move(handler_), std::move(*value_)));
    } else {
      // OnDefaulted() or OnAbandoned() case.
      // TODO(tzik): Handle this.
    }

    delete this;
  }

  ~PromiseStorage() {}

  std::atomic<int> progress_{2};

  scoped_refptr<TaskRunner> task_runner_;
  OnceCallback<void(Value)> handler_;
  Location from_here_;
  Optional<Value> value_;
};

}  // namespace internal

template <typename Value>
std::tuple<Future<Value>, Promise<Value>> Contract() {
  using Storage = internal::PromiseStorage<Value>;
  Storage* storage = new Storage;
  return std::make_tuple(Future<Value>(storage), Promise<Value>(storage));
}

template <typename Value>
class Future {
 public:
  // Future is a move-only type.
  Future() = default;
  Future(Future&&) = default;
  Future& operator=(Future&&) = default;
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;
  ~Future() = default;

  template <typename R>
  Future<R> Chain(scoped_refptr<TaskRunner> task_runner,
                  const Location& from_here,
                  OnceCallback<R(Value)> handler) && {
    Future<R> future;
    Promise<R> promise;
    std::tie(future, promise) = Contract<R>();

    auto relay = [](const Location& from_here, OnceCallback<R(Value)> handler,
                    Promise<R> chained, Value value) {
      std::move(chained).Resolve(from_here,
                                 std::move(handler).Run(std::move(value)));
    };

    std::move(*this).Then(
        std::move(task_runner),
        BindOnce(relay, from_here, std::move(handler), std::move(promise)));
    return future;
  }

  template <typename R>
  Future<R> Chain(const Location& from_here,
                  OnceCallback<R(Value)> handler) && {
    Future<R> future;
    Promise<R> promise;
    std::tie(future, promise) = Contract<R>();

    auto relay = [](const Location& from_here, OnceCallback<R(Value)> handler,
                    Promise<R> chained, Value value) {
      std::move(chained).Resolve(from_here,
                                 std::move(handler).Run(std::move(value)));
    };

    std::move(*this).Then(
        BindOnce(relay, from_here, std::move(handler), std::move(promise)));
    return future;
  }

  void Then(OnceCallback<void(Value)> handler) && {
    std::move(*this).Then(SequencedTaskRunnerHandle::Get(), std::move(handler));
  }

  void Then(scoped_refptr<TaskRunner> task_runner,
            OnceCallback<void(Value)> handler) && {
    DCHECK(storage_);
    internal::PromiseStorage<Value>* storage = storage_.release();
    storage->Attach(std::move(task_runner), std::move(handler));
  }

 private:
  friend std::tuple<Future<Value>, Promise<Value>> Contract<Value>();

  struct AbandonOnScopeOut {
    void operator()(internal::PromiseStorage<Value>* storage) const {
      storage->OnAbandoned();
    }
  };

  explicit Future(internal::PromiseStorage<Value>* storage)
      : storage_(storage) {}

  std::unique_ptr<internal::PromiseStorage<Value>, AbandonOnScopeOut> storage_;
};

template <typename Value>
class Promise {
 public:
  // Promise is a move-only type.
  Promise() = default;
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  ~Promise() {
    if (storage_)
      storage_->OnDefaulted();
  }

  void Resolve(const Location& from_here, Value value) && {
    DCHECK(storage_);
    internal::PromiseStorage<Value>* storage = storage_.release();
    storage->Resolve(from_here, std::move(value));
  }

 private:
  friend std::tuple<Future<Value>, Promise<Value>> Contract<Value>();

  struct DefaultOnScopeOut {
    void operator()(internal::PromiseStorage<Value>* storage) const {
      storage->OnDefaulted();
    }
  };

  explicit Promise(internal::PromiseStorage<Value>* storage)
      : storage_(storage) {}

  std::unique_ptr<internal::PromiseStorage<Value>, DefaultOnScopeOut> storage_;
};

}  // namespace base
