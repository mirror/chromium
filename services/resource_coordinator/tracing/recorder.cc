// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/tracing/recorder.h"

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "services/resource_coordinator/public/interfaces/tracing/tracing.mojom.h"

namespace tracing {

Recorder::Recorder(
    mojom::RecorderRequest request,
    mojom::TraceDataType data_type,
    const base::Closure& on_data_change_callback,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner)
    : is_recording_(true),
      data_type_(data_type),
      on_data_change_callback_(on_data_change_callback),
      background_task_runner_(background_task_runner),
      binding_(this, std::move(request)),
      background_weak_ptr_factory_(this),
      ui_weak_ptr_factory_(this) {
  binding_.set_connection_error_handler(base::BindOnce(
      &Recorder::OnConnectionError, ui_weak_ptr_factory_.GetWeakPtr()));
}

Recorder::~Recorder() {
  ui_weak_ptr_factory_.InvalidateWeakPtrs();
  if (background_task_runner_->RunsTasksInCurrentSequence()) {
    background_weak_ptr_factory_.InvalidateWeakPtrs();
  } else {
    base::WaitableEvent completion(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    background_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&Recorder::ShutdownOnBackgroundThread,
                                  base::Unretained(this), &completion));
    completion.Wait();
  }
}

void Recorder::ShutdownOnBackgroundThread(base::WaitableEvent* completion) {
  background_weak_ptr_factory_.InvalidateWeakPtrs();
  completion->Signal();
}

void Recorder::AddChunk(const std::string& chunk) {
  if (chunk.empty())
    return;
  if (!background_task_runner_->RunsTasksInCurrentSequence()) {
    background_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&Recorder::AddChunk,
                       background_weak_ptr_factory_.GetWeakPtr(), chunk));
    return;
  }
  if (data_type_ != mojom::TraceDataType::STRING && !data_.empty())
    data_.append(",");
  data_.append(chunk);
  on_data_change_callback_.Run();
}

void Recorder::AddMetadata(std::unique_ptr<base::DictionaryValue> metadata) {
  metadata_.MergeDictionary(metadata.get());
}

void Recorder::OnConnectionError() {
  if (!background_task_runner_->RunsTasksInCurrentSequence()) {
    background_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&Recorder::OnConnectionError,
                                  background_weak_ptr_factory_.GetWeakPtr()));
    return;
  }
  is_recording_ = false;
  on_data_change_callback_.Run();
}

}  // namespace tracing
