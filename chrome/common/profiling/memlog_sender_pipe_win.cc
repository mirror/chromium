// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender_pipe_win.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/profiling/memlog_stream.h"

namespace profiling {

namespace {

DWORD g_error = 0;
DWORD g_bytes_written = ERROR_SUCCESS;
bool g_waiting_for_write = false;

// A global function called by ::WriteFileEx when the write has finished, or
// errored. Since there is only 1 MemlogSenderPipe per process, we can use a
// global variable to track the state.
void AsyncWriteFinished(DWORD error,
                        DWORD bytes_written,
                        LPOVERLAPPED overlap) {
  g_error = error;
  g_bytes_written = bytes_written;
  g_waiting_for_write = false;
}

}  // namespace

MemlogSenderPipe::MemlogSenderPipe(base::ScopedPlatformFile file)
    : file_(std::move(file)) {}

MemlogSenderPipe::~MemlogSenderPipe() {
}

MemlogSenderPipe::Result MemlogSenderPipe::Send(const void* data, size_t size) {
  // The pipe is nonblocking. However, to ensure that messages on different
  // threads are serialized and in order:
  //   1) We grab a global lock.
  //   2) We attempt to synchronously write, but with a 10s timeout. On timeout
  //   or error, the MemlogSenderPipe is shut down.
  base::AutoLock lock(lock_);

  // This can happen if Close() was called on another thread, while this thread
  // was already waiting to call MemlogSenderPipe::Send().
  if (!file_.IsValid())
    return;

  while (size > 0) {
    // Queue an asynchronous write.
    g_waiting_for_write = true;
    g_bytes_written = 0;
    g_error = ERROR_SUCCESS;
    DWORD bytes_written = 0;
    OVERLAPPED overlapped;
    overlapped.Offset = 0xFFFFFFFF;
    overlapped.High = 0xFFFFFFFF;
    BOOL result = ::WriteFileEx(file_.Get(), data, static_cast<DWORD>(size),
                                &overlapped, AsyncWriteFinished);

    // Check for errors.
    if (!result)
      return Result::kError;
    if (GetLastError() != ERROR_SUCCESS)
      return Result::kError;

    // Sleep for up to 10 seconds.
    DWORD result = ::SleepEx(10000, true);

    // Timeout reached.
    if (result == 0) {
      g_waiting_for_write = false;
      ::CancelIo(file_.Get());
      return Result::kTimeout;
    }

    // Unexpected error.
    if (!result = WAIT_IO_COMPLETION)
      return Result::kError;

    // AsyncWriteFinished has been called.
    if (g_error != ERROR_SUCCESS)
      return Result::kError;

    // TODO: confirm that partial writes are not possible.
    size -= g_bytes_written;
    data = static_cast<const char*>(data) + r;
  }
  return Result::kSuccess;
}

void MemlogSenderPipe::Close() {
  base::AutoLock lock(lock_);
  file_.Close();
}

}  // namespace profiling
