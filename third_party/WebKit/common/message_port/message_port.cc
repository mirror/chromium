// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/message_port/message_port.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "third_party/WebKit/common/message_port/message_port.mojom.h"
#include "third_party/WebKit/common/message_port/message_port_message_struct_traits.h"

namespace blink_common {

MessagePort::~MessagePort() {}

MessagePort::MessagePort() : state_(new State()) {}

MessagePort::MessagePort(const MessagePort& other) : state_(other.state_) {}

MessagePort& MessagePort::operator=(const MessagePort& other) {
  state_ = other.state_;
  return *this;
}

MessagePort::MessagePort(mojo::ScopedMessagePipeHandle handle)
    : state_(new State(std::move(handle))) {}

const mojo::ScopedMessagePipeHandle& MessagePort::GetHandle() const {
  return state_->handle();
}

mojo::ScopedMessagePipeHandle MessagePort::ReleaseHandle() const {
  state_->StopWatching();
  return state_->TakeHandle();
}

// static
std::vector<mojo::ScopedMessagePipeHandle> MessagePort::ReleaseHandles(
    const std::vector<MessagePort>& ports) {
  std::vector<mojo::ScopedMessagePipeHandle> handles(ports.size());
  for (size_t i = 0; i < ports.size(); ++i)
    handles[i] = ports[i].ReleaseHandle();
  return handles;
}

// static
std::vector<MessagePort> MessagePort::BindHandles(
    std::vector<mojo::ScopedMessagePipeHandle> handles) {
  std::vector<MessagePort> ports(handles.size());
  for (size_t i = 0; i < handles.size(); ++i)
    ports[i] = MessagePort(std::move(handles[i]));
  return ports;
}

void MessagePort::PostMessage(const uint8_t* encoded_message,
                              size_t encoded_message_size,
                              std::vector<MessagePort> ports) {

  MessagePortMessage msg;
  msg.encoded_message =
      mojo::ConstCArray<uint8_t>(encoded_message_size, encoded_message);
  msg.ports = std::move(ports);
  PostMessage<mojom::MessagePortMessage>(&msg);
}

bool MessagePort::GetMessage(std::vector<uint8_t>* encoded_message,
                             std::vector<MessagePort>* ports) {
  MessagePortMessage msg;
  bool success = GetMessage<mojom::MessagePortMessage>(&msg);
  if (!success)
    return false;

  *encoded_message = std::move(msg.owned_encoded_message);
  *ports = std::move(msg.ports);

  return true;
}

void MessagePort::SetCallback(const base::Closure& callback) {
  state_->StopWatching();
  state_->StartWatching(callback);
}

void MessagePort::ClearCallback() {
  state_->StopWatching();
}

MessagePort::State::State() {}

MessagePort::State::State(mojo::ScopedMessagePipeHandle handle)
    : handle_(std::move(handle)) {}

void MessagePort::State::StartWatching(const base::Closure& callback) {
  base::AutoLock lock(lock_);
  DCHECK(!callback_);
  DCHECK(handle_.is_valid());
  callback_ = callback;

  DCHECK(!watcher_handle_.is_valid());
  MojoResult rv = CreateWatcher(&State::CallOnHandleReady, &watcher_handle_);
  DCHECK_EQ(MOJO_RESULT_OK, rv);

  // Balanced in CallOnHandleReady when MOJO_RESULT_CANCELLED is received.
  AddRef();

  // NOTE: An HTML MessagePort does not receive an event to tell it when the
  // peer has gone away, so we only watch for readability here.
  rv = MojoWatch(watcher_handle_.get().value(), handle_.get().value(),
                 MOJO_HANDLE_SIGNAL_READABLE, MOJO_WATCH_CONDITION_SATISFIED,
                 reinterpret_cast<uintptr_t>(this));
  DCHECK_EQ(MOJO_RESULT_OK, rv);

  ArmWatcher();
}

void MessagePort::State::StopWatching() {
  mojo::ScopedWatcherHandle watcher_handle;

  {
    // NOTE: Resetting the watcher handle may synchronously invoke
    // OnHandleReady(), so we don't hold |lock_| while doing that.
    base::AutoLock lock(lock_);
    watcher_handle = std::move(watcher_handle_);
    callback_.Reset();
  }
}

mojo::ScopedMessagePipeHandle MessagePort::State::TakeHandle() {
  base::AutoLock lock(lock_);
  DCHECK(!watcher_handle_.is_valid());
  return std::move(handle_);
}

MessagePort::State::~State() = default;

void MessagePort::State::ArmWatcher() {
  lock_.AssertAcquired();

  if (!watcher_handle_.is_valid())
    return;

  uint32_t num_ready_contexts = 1;
  uintptr_t ready_context;
  MojoResult ready_result;
  MojoHandleSignalsState ready_state;
  MojoResult rv =
      MojoArmWatcher(watcher_handle_.get().value(), &num_ready_contexts,
                     &ready_context, &ready_result, &ready_state);
  if (rv == MOJO_RESULT_OK)
    return;

  // The watcher could not be armed because it would notify immediately.
  DCHECK_EQ(MOJO_RESULT_FAILED_PRECONDITION, rv);
  DCHECK_EQ(1u, num_ready_contexts);
  DCHECK_EQ(reinterpret_cast<uintptr_t>(this), ready_context);

  if (ready_result == MOJO_RESULT_OK) {
    // The handle is already signaled, so we trigger a callback now.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&State::OnHandleReady, this, MOJO_RESULT_OK));
    return;
  }

  if (ready_result == MOJO_RESULT_FAILED_PRECONDITION) {
    DVLOG(1) << this << " MojoArmWatcher failed because of a broken pipe.";
    return;
  }

  NOTREACHED();
}

void MessagePort::State::OnHandleReady(MojoResult result) {
  base::AutoLock lock(lock_);
  if (result == MOJO_RESULT_OK && callback_) {
    callback_.Run();
    ArmWatcher();
  } else {
    // And now his watch is ended.
  }
}

// static
void MessagePort::State::CallOnHandleReady(uintptr_t context,
                                           MojoResult result,
                                           MojoHandleSignalsState signals_state,
                                           MojoWatcherNotificationFlags flags) {
  auto* state = reinterpret_cast<State*>(context);
  if (result == MOJO_RESULT_CANCELLED) {
    // Balanced in MessagePort::State::StartWatching().
    state->Release();
  } else {
    state->OnHandleReady(result);
  }
}

}  // namespace blink_common
