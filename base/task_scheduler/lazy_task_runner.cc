// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/lazy_task_runner.h"

#include "base/lazy_instance.h"
#include "base/task_scheduler/post_task.h"

namespace base {

namespace {

ScopedLazyTaskRunnerList* g_scoped_lazy_task_runner_list = nullptr;

}  // namespace

namespace internal {
template <>
BASE_EXPORT scoped_refptr<SequencedTaskRunner>
LazyTaskRunner<SequencedTaskRunner>::Get() {
  if (internal::NeedsLazyInstance(&private_state_)) {
    scoped_refptr<SequencedTaskRunner> task_runner =
        CreateSequencedTaskRunnerWithTraits(private_traits_);

    // Acquire a reference to the SequencedTaskRunner. The reference will either
    // never be released or be released in Reset(). The reference is not managed
    // by a scoped_refptr because adding a scoped_refptr member to
    // LazySequencedTaskRunner would prevent its static initialization.
    task_runner->AddRef();

    // Reset the TaskRunner when the current ScopedLazyTaskRunnerList is
    // destroyed.
    if (g_scoped_lazy_task_runner_list) {
      g_scoped_lazy_task_runner_list->AddCallback(BindOnce(
          &LazyTaskRunner<SequencedTaskRunner>::Reset, Unretained(this)));
    }

    internal::CompleteLazyInstance(
        &private_state_,
        reinterpret_cast<subtle::AtomicWord>(task_runner.get()), nullptr,
        nullptr);
  }

  return scoped_refptr<SequencedTaskRunner>(
      reinterpret_cast<SequencedTaskRunner*>(
          subtle::Acquire_Load(&private_state_)));
}

template <>
BASE_EXPORT void LazyTaskRunner<SequencedTaskRunner>::Reset() {
  subtle::AtomicWord state = subtle::Acquire_Load(&private_state_);

  // Calling Reset() while another thread is calling Get() is not supported.
  DCHECK_NE(state, kLazyInstanceStateCreating);

  // Return if no reference is held by this instance.
  if (state == 0)
    return;

  // Release the reference acquired in Get().
  SequencedTaskRunner* task_runner =
      reinterpret_cast<SequencedTaskRunner*>(state);
  task_runner->Release();

  // Clear the state.
  subtle::NoBarrier_Store(&private_state_, 0);
}

}  // namespace internal

void ScopedLazyTaskRunnerList::AddCallback(OnceClosure callback) {
  internal::AutoSchedulerLock auto_lock(lock_);
  callbacks_.push_back(std::move(callback));
}

}  // namespace base
