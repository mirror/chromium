// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/promise_internal.h"

namespace base {
namespace internal {

FinallyPromise::FinallyPromise(Optional<Location> from_here,
                               scoped_refptr<SequencedTaskRunner> task_runner,
                               scoped_refptr<PromiseBase> prerequisite,
                               OnceClosure finally_callback)
    : PromiseBase(std::move(from_here),
                  std::move(task_runner),
                  PromiseBase::State::UNRESOLVED,
                  PromiseBase::PrerequisitePolicy::FINALLY,
                  {std::move(prerequisite)}),
      finally_callback_(std::move(finally_callback)) {}

FinallyPromise::~FinallyPromise() {}

bool FinallyPromise::HasOnResolveExecutor() {
  return true;
}

bool FinallyPromise::HasOnRejectExecutor() {
  return true;
}

const RejectValue* FinallyPromise::GetRejectValue() const {
  NOTREACHED();
  return nullptr;
}

void FinallyPromise::PropagateRejectValue(const RejectValue& reject_reason) {
  NOTREACHED();
}

FinallyPromise::ExecutorResult FinallyPromise::RunExecutor() {
  std::move(finally_callback_).Run();
  return ExecutorResult(PromiseBase::State::AFTER_FINALLY);
}

}  // namespace internal
}  // namespace base
