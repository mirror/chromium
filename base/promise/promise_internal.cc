// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/promise/promise_internal.h"

#include "base/lazy_instance.h"

namespace base {
namespace internal {

struct SynchronousTaskRunner::Holder {
  SynchronousTaskRunner task_runner_;
  const scoped_refptr<SynchronousTaskRunner> ref_;

  Holder() : ref_(&task_runner_) {}
  ~Holder() = delete;
};

// static
const scoped_refptr<SynchronousTaskRunner>&
SynchronousTaskRunner::GetInstance() {
  static LazyInstance<Holder>::Leaky instance = LAZY_INSTANCE_INITIALIZER;
  return instance.Pointer()->ref_;
}

SynchronousTaskRunner::SynchronousTaskRunner() = default;
SynchronousTaskRunner::~SynchronousTaskRunner() = default;

bool SynchronousTaskRunner::RunsTasksInCurrentSequence() const {
  return true;
}

bool SynchronousTaskRunner::PostDelayedTask(const Location& from_here,
                                            OnceClosure task,
                                            base::TimeDelta delay) {
  DCHECK(delay.is_zero());
  std::move(task).Run();
  return true;
}

}  // namespace internal
}  // namespace base
