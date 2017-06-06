// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/lazy_task_runner.h"

#include <utility>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/task_scheduler/post_task.h"

namespace base {
namespace internal {

namespace {
ScopedLazyTaskRunnerList* g_scoped_lazy_task_runner_list = nullptr;
}  // namespace

template <typename TaskRunnerType, bool com_sta>
void LazyTaskRunner<TaskRunnerType, com_sta>::Reset() {
  subtle::AtomicWord state = subtle::Acquire_Load(&state_);

  // Calling Reset() while another thread is calling Get() is not supported.
  DCHECK_NE(state, kLazyInstanceStateCreating);

  // Return if no reference is held by this instance.
  if (state == 0)
    return;

  // Release the reference acquired in Get().
  SequencedTaskRunner* task_runner = reinterpret_cast<TaskRunnerType*>(state);
  task_runner->Release();

  // Clear the state.
  subtle::NoBarrier_Store(&state_, 0);
}

template <>
scoped_refptr<SequencedTaskRunner>
LazyTaskRunner<SequencedTaskRunner, false>::Create() {
  // It is invalid to specify a SingleThreadTaskRunnerThreadMode with a
  // LazySequencedTaskRunner.
  DCHECK_EQ(thread_mode_, SingleThreadTaskRunnerThreadMode::SHARED);

  return CreateSequencedTaskRunnerWithTraits(traits_);
}

template <>
scoped_refptr<SingleThreadTaskRunner>
LazyTaskRunner<SingleThreadTaskRunner, false>::Create() {
  return CreateSingleThreadTaskRunnerWithTraits(traits_, thread_mode_);
}

#if defined(OS_WIN)
template <>
scoped_refptr<SingleThreadTaskRunner>
LazyTaskRunner<SingleThreadTaskRunner, true>::Create() {
  return CreateCOMSTATaskRunnerWithTraits(traits_, thread_mode_);
}
#endif

template <typename TaskRunnerType, bool com_sta>
scoped_refptr<TaskRunnerType> LazyTaskRunner<TaskRunnerType, com_sta>::Get() {
  if (internal::NeedsLazyInstance(&state_)) {
    scoped_refptr<TaskRunnerType> task_runner = Create();

    // Acquire a reference to the TaskRunner. The reference will either never be
    // released or be released in Reset(). The reference is not managed by a
    // scoped_refptr because adding a scoped_refptr member to LazyTaskRunner
    // would prevent its static initialization.
    task_runner->AddRef();

    // Reset this instance when the current ScopedLazyTaskRunnerList is
    // destroyed, if any.
    if (g_scoped_lazy_task_runner_list) {
      g_scoped_lazy_task_runner_list->AddCallback(
          BindOnce(&std::remove_reference<decltype(*this)>::type::Reset,
                   Unretained(this)));
    }

    internal::CompleteLazyInstance(
        &state_, reinterpret_cast<subtle::AtomicWord>(task_runner.get()),
        nullptr, nullptr);
  }

  return scoped_refptr<TaskRunnerType>(
      reinterpret_cast<TaskRunnerType*>(subtle::Acquire_Load(&state_)));
}

template class LazyTaskRunner<SequencedTaskRunner, false>;
template class LazyTaskRunner<SingleThreadTaskRunner, false>;

#if defined(OS_WIN)
template class LazyTaskRunner<SingleThreadTaskRunner, true>;
#endif

ScopedLazyTaskRunnerList::ScopedLazyTaskRunnerList() {
  DCHECK(!g_scoped_lazy_task_runner_list);
  g_scoped_lazy_task_runner_list = this;
}

ScopedLazyTaskRunnerList::~ScopedLazyTaskRunnerList() {
  internal::AutoSchedulerLock auto_lock(lock_);
  for (auto& callback : callbacks_)
    std::move(callback).Run();
  g_scoped_lazy_task_runner_list = nullptr;
}

void ScopedLazyTaskRunnerList::AddCallback(OnceClosure callback) {
  internal::AutoSchedulerLock auto_lock(lock_);
  callbacks_.push_back(std::move(callback));
}

}  // namespace internal
}  // namespace base
