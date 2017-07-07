// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SCOPED_CALLBACK_RUNNER_H_
#define MEDIA_BASE_SCOPED_CALLBACK_RUNNER_H_

#include <memory>
#include <tuple>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "base/tuple.h"

// This is a helper utility to wrap a base::OnceCallback such that if the
// callback is destructed before it has a chance to run (e.g. the callback is
// bound into a task in a task runner and the task is dropped), it will be run
// with the parameters passed into ScopedCallbackRunner.
//
// Example:
//   foo->DoWorkAndReturnResult(
//     ScopedCallbackRunner(base::Bind(&Foo::OnResult, this), false));
//
// If the callback is destructed without running, it'll be run with "false".

namespace media {
namespace internal {

template <typename CallbackType, typename Tuple, size_t... Ns>
inline void DispatchToCallbackImpl(CallbackType callback,
                                   Tuple&& args,
                                   base::IndexSequence<Ns...>) {
  std::move(callback).Run(
      std::move(std::get<Ns>(std::forward<Tuple>(args)))...);
}

template <typename CallbackType, typename Tuple>
inline void DispatchToCallback(CallbackType callback, Tuple&& args) {
  DispatchToCallbackImpl(std::move(callback), std::forward<Tuple>(args),
                         base::MakeIndexSequenceForTuple<Tuple>());
}

// First, tell the compiler ScopedCallbackRunnerHelper is a struct template with
// one type parameter.  Then define specializations where the type is a function
// returning void and taking zero or more arguments.
template <typename Signature>
class ScopedCallbackRunnerHelper;

template <typename... Args>
class ScopedCallbackRunnerHelper<void(Args...)> {
 public:
  using CallbackType = base::OnceCallback<void(Args...)>;

  ScopedCallbackRunnerHelper(CallbackType callback, Args... args)
      : callback_(std::move(callback)), args_(std::forward<Args>(args)...) {
    DCHECK(callback_);
  }

  inline void Run(Args... args) {
    std::move(callback_).Run(std::forward<Args>(args)...);
  }

  ~ScopedCallbackRunnerHelper() {
    if (callback_)
      DispatchToCallback(std::move(callback_), args_);
  }

 private:
  CallbackType callback_;
  std::tuple<Args...> args_;
};

}  // namespace internal

// Currently ScopedCallbackRunner only supports base::OnceCallback. If needed,
// we easily add a specialization to support base::RepeatingCallback as well.
template <typename T, typename... Args>
inline base::OnceCallback<T> ScopedCallbackRunner(base::OnceCallback<T> cb,
                                                  Args... args) {
  return base::BindOnce(
      &internal::ScopedCallbackRunnerHelper<T>::Run,
      base::MakeUnique<internal::ScopedCallbackRunnerHelper<T>>(
          std::move(cb), std::move(args)...));
}

}  // namespace media

#endif  // MEDIA_BASE_SCOPED_CALLBACK_RUNNER_H_
