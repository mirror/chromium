// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/fetch/BytesConsumerForMojoDataPipe.h"

#include "core/dom/ExecutionContext.h"
#include "platform/WebTaskRunner.h"
#include "platform/wtf/Functional.h"
#include "public/platform/TaskType.h"
#include "public/platform/WebTraceLocation.h"

#include <string.h>
#include <algorithm>

namespace blink {

BytesConsumerForMojoDataPipe::BytesConsumerForMojoDataPipe(
    ExecutionContext* execution_context,
    mojo::ScopedDataPipeConsumerHandle consumer_handle)
    : execution_context_(execution_context),
      consumer_handle_(std::move(consumer_handle)),
      handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL) {
  handle_watcher_.Watch(
      consumer_handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&BytesConsumerForMojoDataPipe::OnHandleGotReadable,
                 base::Unretained(this)));
  handle_watcher_.ArmOrNotify();
}

BytesConsumerForMojoDataPipe::~BytesConsumerForMojoDataPipe() {}

BytesConsumer::Result BytesConsumerForMojoDataPipe::BeginRead(
    const char** buffer,
    size_t* available) {
  DCHECK(!is_in_two_phase_read_);
  *buffer = nullptr;
  *available = 0;
  if (state_ == InternalState::kClosed)
    return Result::kDone;
  if (state_ == InternalState::kErrored)
    return Result::kError;

  uint32_t size_to_pass = 0;
  MojoReadDataFlags flags_to_pass = MOJO_READ_DATA_FLAG_NONE;
  MojoResult rv = consumer_handle_->BeginReadData(
      reinterpret_cast<const void**>(buffer), &size_to_pass, flags_to_pass);
  switch (rv) {
    case MOJO_RESULT_OK:
      *available = size_to_pass;
      is_in_two_phase_read_ = true;
      return Result::kOk;
    case MOJO_RESULT_FAILED_PRECONDITION:
      Close();
      return Result::kDone;
    case MOJO_RESULT_SHOULD_WAIT:
      return Result::kShouldWait;
    default:
      break;
  }
  SetError();
  return Result::kError;
}

BytesConsumer::Result BytesConsumerForMojoDataPipe::EndRead(size_t read_size) {
  DCHECK(is_in_two_phase_read_);
  is_in_two_phase_read_ = false;
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  MojoResult rv = consumer_handle_->EndReadData(read_size);
  if (rv != MOJO_RESULT_OK) {
    has_pending_notification_ = false;
    SetError();
    return Result::kError;
  }

  handle_watcher_.ArmOrNotify();
  if (has_pending_notification_) {
    has_pending_notification_ = false;
    execution_context_->GetTaskRunner(TaskType::kNetworking)
        ->PostTask(BLINK_FROM_HERE,
                   WTF::Bind(&BytesConsumerForMojoDataPipe::Notify,
                             WrapPersistent(this)));
  }
  return Result::kOk;
}

void BytesConsumerForMojoDataPipe::SetClient(BytesConsumer::Client* client) {
  DCHECK(!client_);
  DCHECK(client);
  if (state_ == InternalState::kReadable || state_ == InternalState::kWaiting)
    client_ = client;
}

void BytesConsumerForMojoDataPipe::ClearClient() {
  client_ = nullptr;
}

void BytesConsumerForMojoDataPipe::Cancel() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kReadable || state_ == InternalState::kWaiting) {
    // We don't want the client to be notified in this case.
    BytesConsumer::Client* client = client_;
    client_ = nullptr;
    Close();
    client_ = client;
  }
}

BytesConsumer::PublicState BytesConsumerForMojoDataPipe::GetPublicState()
    const {
  return GetPublicStateFromInternalState(state_);
}

void BytesConsumerForMojoDataPipe::OnHandleGotReadable(MojoResult) {
  DCHECK(state_ == InternalState::kWaiting ||
         state_ == InternalState::kReadable);
  if (is_in_two_phase_read_) {
    has_pending_notification_ = true;
    return;
  }

  mojo::HandleSignalsState state = consumer_handle_->QuerySignalsState();
  if (state.never_readable())
    Close();
  if (client_)
    client_->OnStateChange();
}

void BytesConsumerForMojoDataPipe::Trace(blink::Visitor* visitor) {
  visitor->Trace(execution_context_);
  visitor->Trace(client_);
  BytesConsumer::Trace(visitor);
}

void BytesConsumerForMojoDataPipe::Close() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kClosed)
    return;
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  state_ = InternalState::kClosed;
  handle_watcher_.Cancel();
  ClearClient();
}

void BytesConsumerForMojoDataPipe::SetError() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kErrored)
    return;
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  state_ = InternalState::kErrored;
  handle_watcher_.Cancel();
  error_ = Error("error");
  ClearClient();
}

void BytesConsumerForMojoDataPipe::Notify() {
  if (state_ == InternalState::kClosed || state_ == InternalState::kErrored)
    return;
  OnHandleGotReadable(MOJO_RESULT_OK);
}

}  // namespace blink
