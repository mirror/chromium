// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/promise_internal.h"

namespace base {

FinallyPromise::FinallyPromise(
    Optional<Location> from_here,
    scoped_refptr<SequencedTaskRunner> task_runner,
    std::vector<scoped_refptr<PromiseBase>> prerequisites,
    OnceClosure finally_callback)
    : PromiseBase(std::move(from_here),
                  std::move(task_runner),
                  PromiseBase::State::UNRESOLVED,
                  PromiseBase::PrerequisitePolicy::FINALLY,
                  std::move(prerequisites)),
      finally_callback_(std::move(finally_callback)) {
  ExecuteIfPossible();
}

FinallyPromise::~FinallyPromise() {}

bool FinallyPromise::HasOnResolveExecutor() {
  return true;
}

bool FinallyPromise::HasOnRejectExecutor() {
  return true;
}

const Value* FinallyPromise::GetRejectValue() const {
  NOTREACHED();
  return nullptr;
}

void FinallyPromise::PropagateRejectValue(const Value& reject_reason) {
  NOTREACHED();
}

FinallyPromise::ExecutorResult FinallyPromise::RunExecutor() {
  std::move(finally_callback_).Run();
  return ExecutorResult(PromiseBase::State::RESOLVED);
}

}  // namespace base
