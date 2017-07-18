// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_SCOPED_RESULT_CALLBACK_H_
#define MEDIA_CAPTURE_VIDEO_SCOPED_RESULT_CALLBACK_H_

#include <tuple>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/macros.h"

namespace media {

namespace internal {

template <typename CallbackType, typename RunType>
struct RunWithDefaultArgsImpl;

template <typename CallbackType, typename... Args>
struct RunWithDefaultArgsImpl<CallbackType, void(Args...)> {
  static void Run(Args... args, CallbackType callback) {
    std::move(callback).Run(std::forward<Args>(args)...);
  }
};

}  // namespace internal

template <typename... Args>
std::tuple<Args&&...> DefaultArgs(Args&&... args) {
  return std::forward_as_tuple(std::forward<Args>(args)...);
}

// This class guarantees that |callback_| has either been called or will pass it
// to |on_error_callback_| on destruction. Inspired by ScopedWebCallbacks<>.
template <typename CallbackType>
class ScopedResultCallback {
 public:
  using OnErrorCallback = base::OnceCallback<void(CallbackType)>;

  ScopedResultCallback(CallbackType callback, OnErrorCallback on_error_callback)
      : callback_(std::move(callback)),
        on_error_callback_(std::move(on_error_callback)) {}

  template <typename... Args>
  ScopedResultCallback(CallbackType callback, std::tuple<Args...> args)
      : callback_(std::move(callback)),
        on_error_callback_(this->MakeErrorCallbackWithDefaultArgs(
            std::move(args),
            base::MakeIndexSequence<sizeof...(Args)>())) {}

  ~ScopedResultCallback() {
    if (!callback_.is_null())
      std::move(on_error_callback_).Run(std::move(callback_));
  }

  ScopedResultCallback(ScopedResultCallback&& other) = default;
  ScopedResultCallback& operator=(ScopedResultCallback&& other) = default;

  template <typename... Args>
  void Run(Args&&... args) {
    on_error_callback_.Reset();
    std::move(callback_).Run(std::forward<Args>(args)...);
  }

 private:
  template <typename ArgsTuple, size_t... Ns>
  OnErrorCallback MakeErrorCallbackWithDefaultArgs(ArgsTuple&& args,
                                                   base::IndexSequence<Ns...>) {
    using Impl =
        internal::RunWithDefaultArgsImpl<CallbackType,
                                         typename CallbackType::RunType>;
    return base::BindOnce(&Impl::Run,
                          std::get<Ns>(std::forward<ArgsTuple>(args))...);
  }

  CallbackType callback_;
  OnErrorCallback on_error_callback_;

  DISALLOW_COPY_AND_ASSIGN(ScopedResultCallback);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_SCOPED_RESULT_CALLBACK_H_
