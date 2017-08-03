// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/sync_socket.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#include "base/logging.h"

namespace base {

SyncSocket::SyncSocket() {}
SyncSocket::SyncSocket(Handle handle) : handle_(std::move(handle)) {}
SyncSocket::~SyncSocket() {}

// static
bool SyncSocket::CreatePair(SyncSocket* socket_a, SyncSocket* socket_b) {
  return false;
}

// static
SyncSocket::Handle SyncSocket::UnwrapHandle(
    const SyncSocket::TransitDescriptor& descriptor) {
  // TODO(xians): Still unclear how NaCl uses SyncSocket.
  // See http://crbug.com/409656
  NOTIMPLEMENTED();
  return Handle();
}

bool SyncSocket::PrepareTransitDescriptor(
    ProcessHandle peer_process_handle,
    SyncSocket::TransitDescriptor* descriptor) {
  // TODO(xians): Still unclear how NaCl uses SyncSocket.
  // See http://crbug.com/409656
  NOTIMPLEMENTED();
  return false;
}

void SyncSocket::Close() {
  handle_.reset();
}

size_t SyncSocket::Send(const void* buffer, size_t length) {
  const ssize_t bytes_written = write(handle_.get(), buffer, length);
  return bytes_written > 0 ? bytes_written : 0;
}

size_t SyncSocket::Receive(void* buffer, size_t length) {
  const ssize_t bytes_read = read(handle_.get(), buffer, length);
  return bytes_read > 0 ? bytes_read : 0;
}

size_t SyncSocket::ReceiveWithTimeout(void* buffer, size_t length, TimeDelta) {
  NOTIMPLEMENTED();
  return 0;
}

size_t SyncSocket::Peek() {
  NOTIMPLEMENTED();
  return 0;
}

SyncSocket::Handle SyncSocket::Release() {
  return std::move(handle_);
}

CancelableSyncSocket::CancelableSyncSocket() {}

CancelableSyncSocket::CancelableSyncSocket(Handle handle)
    : SyncSocket(std::move(handle)) {}

size_t CancelableSyncSocket::Send(const void* buffer, size_t length) {
  return SyncSocket::Send(buffer, length);
}

bool CancelableSyncSocket::Shutdown() {
  SyncSocket::Close();
  return true;
}

// static
bool CancelableSyncSocket::CreatePair(CancelableSyncSocket* socket_a,
                                      CancelableSyncSocket* socket_b) {
  return SyncSocket::CreatePair(socket_a, socket_b);
}

}  // namespace base
