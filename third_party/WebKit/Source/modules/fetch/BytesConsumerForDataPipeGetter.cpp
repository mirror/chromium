// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/fetch/BytesConsumerForDataPipeGetter.h"

#include <algorithm>
#include <string>

#include "core/dom/ExecutionContext.h"
#include "platform/WebTaskRunner.h"
#include "platform/wtf/Functional.h"
#include "public/platform/TaskType.h"
#include "public/platform/WebTraceLocation.h"

namespace blink {

BytesConsumerForDataPipeGetter::BytesConsumerForDataPipeGetter(
    ExecutionContext* execution_context,
    network::mojom::blink::DataPipeGetterPtr data_pipe_getter)
    : execution_context_(execution_context),
      data_pipe_getter_(std::move(data_pipe_getter)),
      handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL) {
  mojo::DataPipe data_pipe;
  data_pipe_getter_->Read(
      std::move(data_pipe.producer_handle),
      ConvertToBaseCallback(
          WTF::Bind(&BytesConsumerForDataPipeGetter::DataPipeGetterCallback,
                    WrapWeakPersistent(this))));
  consumer_handle_ = std::move(data_pipe.consumer_handle);
  handle_watcher_.Watch(
      consumer_handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      WTF::BindRepeating(&BytesConsumerForDataPipeGetter::OnHandleGotReadable,
                         WrapWeakPersistent(this)));
  handle_watcher_.ArmOrNotify();
}

BytesConsumer::Result BytesConsumerForDataPipeGetter::BeginRead(
    const char** buffer,
    size_t* available) {
  DCHECK(!is_in_two_phase_read_);
  *buffer = nullptr;
  *available = 0;
  if (state_ == InternalState::kClosed) {
    return GetPublicState() == PublicState::kClosed ? Result::kDone
                                                    : Result::kShouldWait;
  }
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
      return GetPublicState() == PublicState::kClosed ? Result::kDone
                                                      : Result::kShouldWait;
    case MOJO_RESULT_SHOULD_WAIT:
      return Result::kShouldWait;
    default:
      break;
  }
  SetError();
  return Result::kError;
}

BytesConsumer::Result BytesConsumerForDataPipeGetter::EndRead(
    size_t read_size) {
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
                   WTF::Bind(&BytesConsumerForDataPipeGetter::Notify,
                             WrapPersistent(this)));
  }
  return Result::kOk;
}

void BytesConsumerForDataPipeGetter::SetClient(BytesConsumer::Client* client) {
  DCHECK(!client_);
  DCHECK(client);
  switch (GetPublicState()) {
    case PublicState::kClosed:
    case PublicState::kErrored:
      return;
    case PublicState::kReadableOrWaiting:
      client_ = client;
      return;
  }
}

void BytesConsumerForDataPipeGetter::ClearClient() {
  client_ = nullptr;
}

void BytesConsumerForDataPipeGetter::Cancel() {
  DCHECK(!is_in_two_phase_read_);
  switch (GetPublicState()) {
    case PublicState::kClosed:
    case PublicState::kErrored:
      // Cancel() should do nothing when already closed or errored.
      break;
    case PublicState::kReadableOrWaiting:
      // Otherwise, Cancel() closes.
      state_ = InternalState::kClosed;
      // Pretend the callback was called so we reach PublicState closed.
      was_data_pipe_getter_callback_called_ = true;
      handle_watcher_.Cancel();
      DCHECK_EQ(GetPublicState(), PublicState::kClosed);
      break;
  }
}

BytesConsumer::PublicState BytesConsumerForDataPipeGetter::GetPublicState()
    const {
  switch (state_) {
    case InternalState::kReadable:
    case InternalState::kWaiting:
      return PublicState::kReadableOrWaiting;
    case InternalState::kClosed:
      return was_data_pipe_getter_callback_called_
                 ? PublicState::kClosed
                 : PublicState::kReadableOrWaiting;
    case InternalState::kErrored:
      return PublicState::kErrored;
  }
  NOTREACHED();
  return PublicState::kReadableOrWaiting;
}

void BytesConsumerForDataPipeGetter::DataPipeGetterCallback(int32_t status,
                                                            uint64_t size) {
  LOG(ERROR) << "ReadCallback: " << status << ", " << size;
  was_data_pipe_getter_callback_called_ = true;
  if (status != 0) {  // net::OK
    SetError();
    return;
  }

  if (state_ == InternalState::kClosed && client_) {
    client_->OnStateChange();
    ClearClient();
  }
}

void BytesConsumerForDataPipeGetter::OnHandleGotReadable(MojoResult) {
  DCHECK(state_ == InternalState::kWaiting ||
         state_ == InternalState::kReadable);
  if (is_in_two_phase_read_) {
    has_pending_notification_ = true;
    return;
  }

  mojo::HandleSignalsState state = consumer_handle_->QuerySignalsState();
  BytesConsumer::Client* client = client_;
  if (state.never_readable())
    Close();
  if (client)
    client->OnStateChange();
}

void BytesConsumerForDataPipeGetter::Trace(blink::Visitor* visitor) {
  visitor->Trace(execution_context_);
  visitor->Trace(client_);
  BytesConsumer::Trace(visitor);
}

void BytesConsumerForDataPipeGetter::Close() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kClosed)
    return;
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  state_ = InternalState::kClosed;
  handle_watcher_.Cancel();
  if (was_data_pipe_getter_callback_called_) {
    // Client won't get any more messages. The caller of Close() must invoke
    // OnStateChanged() if neeeded.
    ClearClient();
  }
}

void BytesConsumerForDataPipeGetter::SetError() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kErrored)
    return;
  state_ = InternalState::kErrored;
  handle_watcher_.Cancel();
  error_ = Error("error");
  ClearClient();
}

void BytesConsumerForDataPipeGetter::Notify() {
  if (state_ == InternalState::kClosed || state_ == InternalState::kErrored)
    return;
  OnHandleGotReadable(MOJO_RESULT_OK);
}

}  // namespace blink
