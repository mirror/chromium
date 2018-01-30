// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/promise_executors.h"

namespace base {
namespace internal {

FinallyExecutor::FinallyExecutor(OnceClosure finally_callback)
    : finally_callback_(std::move(finally_callback)) {}

FinallyExecutor::~FinallyExecutor() {}

void FinallyExecutor::Execute(AbstractPromise* promise) {
  std::move(finally_callback_).Run();
  promise->state_ = AbstractPromise::State::AFTER_FINALLY;
}

}  // namespace internal
}  // namespace base
