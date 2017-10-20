// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CALLBACK_WITH_DELETE_H_
#define BASE_CALLBACK_WITH_DELETE_H_

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"

// This is a helper utility to wrap a base::OnceCallback such that if the
// callback is destructed before it has a chance to run (e.g. the callback is
// bound into a task and the task is dropped), it will be run with the
// default arguments passed into CallbackWithDefault.
//
// Example:
//   foo->DoWorkAndReturnResult(
//     CallbackWithDefault(base::BindOnce(&Foo::OnResult, this), false));
//
// If the callback is destructed without running, it'll be run with "false".
//
// If you want to make sure a base::RepeatingCallback is always run, consider
// switching to use base::OnceCallback. If that is not possible, you can use
// ToOnceCallback() to convert it to a OnceCallback.
//
// Example:
//   foo->DoWorkAndReturnResult(
//     CallbackWithDefault(ToOnceCallback(repeating_cb), false));

namespace base {
namespace internal {

// First, tell the compiler CallbackWithDeleteHelper is a class template with
// one type parameter. Then define specializations where the type is a function
// returning void and taking zero or more arguments.
template <typename Signature>
class CallbackWithDeleteHelper;

// Only support callbacks that return void because otherwise it is odd to call
// the callback in the destructor and drop the return value immediately.
template <typename... Args>
class CallbackWithDeleteHelper<void(Args...)> {
 public:
  using CallbackType = base::OnceCallback<void(Args...)>;

  // Bound arguments may be different to the callback signature when wrappers
  // are used, e.g. in base::Owned and base::Unretained case, they are
  // OwnedWrapper and UnretainedWrapper. Use BoundArgs to help handle this.
  template <typename... BoundArgs>
  explicit CallbackWithDeleteHelper(CallbackType callback, BoundArgs&&... args)
      : callback_(std::move(callback)) {
    delete_callback_ =
        base::BindOnce(&CallbackWithDeleteHelper::Run, base::Unretained(this),
                       std::forward<BoundArgs>(args)...);
  }

  // The first int param acts to disambiguate this constructor from the template
  // constructor above. The precendent is C++'s own operator++(int) vs
  // operator++() to distinguish post-increment and pre-increment.
  CallbackWithDeleteHelper(int ignored,
                           CallbackType callback,
                           base::OnceClosure delete_callback)
      : callback_(std::move(callback)),
        delete_callback_(std::move(delete_callback)) {}

  ~CallbackWithDeleteHelper() {
    if (delete_callback_)
      std::move(delete_callback_).Run();
  }

  void Run(Args... args) {
    delete_callback_.Reset();
    std::move(callback_).Run(std::forward<Args>(args)...);
  }

 private:
  CallbackType callback_;
  base::OnceClosure delete_callback_;

  DISALLOW_COPY_AND_ASSIGN(CallbackWithDeleteHelper);
};

}  // namespace internal

template <typename T, typename... Args>
inline base::OnceCallback<T> CallbackWithDelete(base::OnceCallback<T> cb,
                                                base::OnceClosure delete_cb) {
  return base::BindOnce(&internal::CallbackWithDeleteHelper<T>::Run,
                        base::MakeUnique<internal::CallbackWithDeleteHelper<T>>(
                            0, std::move(cb), std::move(delete_cb)));
}

template <typename T, typename... Args>
inline base::OnceCallback<T> CallbackWithDefault(base::OnceCallback<T> cb,
                                                 Args&&... args) {
  return base::BindOnce(&internal::CallbackWithDeleteHelper<T>::Run,
                        base::MakeUnique<internal::CallbackWithDeleteHelper<T>>(
                            std::move(cb), std::forward<Args>(args)...));
}

// Converts a repeating callback to a once callback with the same signature so
// that it can be used with CallbackWithDefault.
template <typename T>
base::OnceCallback<T> ToOnceCallback(base::RepeatingCallback<T> cb) {
  return static_cast<base::OnceCallback<T>>(std::move(cb));
}

}  // namespace base

#endif  // BASE_CALLBACK_WITH_DELETE_H_
