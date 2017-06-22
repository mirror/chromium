// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/child_process_crasher_impl.h"

#include <utility>

#include "base/debug/crash_logging.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

void ChildProcessCrasherImpl::Create(
    scoped_refptr<base::SingleThreadTaskRunner> io_thread,
    const service_manager::BindSourceInfo&,
    mojom::ChildProcessCrasherRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<ChildProcessCrasherImpl>(std::move(io_thread)),
      std::move(request));
}

ChildProcessCrasherImpl::ChildProcessCrasherImpl(
    scoped_refptr<base::SingleThreadTaskRunner> io_thread)
    : io_thread_(std::move(io_thread)) {
  DCHECK(io_thread_);
  DCHECK(io_thread_->RunsTasksInCurrentSequence());
}

ChildProcessCrasherImpl::~ChildProcessCrasherImpl() = default;

void ChildProcessCrasherImpl::Crash(mojom::CrashKeysPtr crash_keys) {
  DCHECK(io_thread_->RunsTasksInCurrentSequence());

  // Matches //chrome/common/crash_keys.cc.
  base::debug::SetCrashKeyValue(
      "free-memory-ratio-in-percent",
      base::IntToString(crash_keys->free_memory_ratio));
  base::debug::SetCrashKeyValue(
      "free-memory-bytes", base::Int64ToString(crash_keys->free_memory_bytes));
  IMMEDIATE_CRASH();
}

}  // namespace content
