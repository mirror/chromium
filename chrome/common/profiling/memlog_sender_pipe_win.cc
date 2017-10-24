// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender_pipe.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
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
void WINAPI AsyncWriteFinished(DWORD error,
                               DWORD bytes_written,
                               LPOVERLAPPED overlap) {
  g_error = error;
  g_bytes_written = bytes_written;
  g_waiting_for_write = false;
}

}  // namespace

MemlogSenderPipe::PipePair::PipePair() = default;
MemlogSenderPipe::PipePair::PipePair(PipePair&& other) = default;

MemlogSenderPipe::PipePair MemlogSenderPipe::CreatePipes() {
  PipePair pipes;
  std::wstring pipe_name = base::StringPrintf(
      L"\\\\.\\pipe\\profiling.%u.%u.%I64u", GetCurrentProcessId(),
      GetCurrentThreadId(), base::RandUint64());

  DWORD kOpenMode =
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE;
  const DWORD kPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;
  pipes.receiver.reset(mojo::edk::PlatformHandle(CreateNamedPipeW(
      pipe_name.c_str(), kOpenMode, kPipeMode,
      1,          // Max instances.
      kPipeSize,  // Out buffer size.
      kPipeSize,  // In buffer size.
      5000,  // Timeout in milliseconds for connecting the receiving pipe. Has
             // nothing to do with Send() timeout..
      nullptr)));  // Default security descriptor.
  PCHECK(pipes.receiver.is_valid());

  const DWORD kDesiredAccess = GENERIC_WRITE;
  // The SECURITY_ANONYMOUS flag means that the server side cannot impersonate
  // the client.
  DWORD kFlags =
      SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS | FILE_FLAG_OVERLAPPED;

  // Allow the handle to be inherited by child processes.
  SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES),
                                             nullptr, TRUE};
  pipes.sender.reset(mojo::edk::PlatformHandle(
      CreateFileW(pipe_name.c_str(), kDesiredAccess,
                  0,  // No sharing.
                  &security_attributes, OPEN_EXISTING, kFlags,
                  nullptr)));  // No template file.
  PCHECK(pipes.sender.is_valid());

  // Since a client has connected, ConnectNamedPipe() should return zero and
  // GetLastError() should return ERROR_PIPE_CONNECTED.
  CHECK(!ConnectNamedPipe(pipes.receiver.get().handle, nullptr));
  PCHECK(GetLastError() == ERROR_PIPE_CONNECTED);
  return pipes;
}

MemlogSenderPipe::MemlogSenderPipe(base::ScopedPlatformFile file)
    : file_(std::move(file)) {}

MemlogSenderPipe::~MemlogSenderPipe() {
}

MemlogSenderPipe::Result MemlogSenderPipe::Send(const void* data,
                                                size_t size,
                                                int timeout_ms) {
  // The pipe is nonblocking. However, to ensure that messages on different
  // threads are serialized and in order:
  //   1) We grab a global lock.
  //   2) We attempt to synchronously write, but with a timeout. On timeout
  //   or error, the MemlogSenderPipe is shut down.
  base::AutoLock lock(lock_);

  // This can happen if Close() was called on another thread, while this thread
  // was already waiting to call MemlogSenderPipe::Send().
  if (!file_.IsValid())
    return Result::kError;

  // Queue an asynchronous write.
  g_waiting_for_write = true;
  g_bytes_written = 0;
  g_error = ERROR_SUCCESS;
  OVERLAPPED overlapped;
  overlapped.Offset = 0xFFFFFFFF;
  overlapped.OffsetHigh = 0xFFFFFFFF;
  BOOL write_result = ::WriteFileEx(file_.Get(), data, static_cast<DWORD>(size),
                                    &overlapped, AsyncWriteFinished);

  // Check for errors.
  if (!write_result)
    return Result::kError;

  // The documentation for ::WriteFileEx
  // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365748(v=vs.85).aspx
  // claims that we need to check GetLastError() even on success. This is
  // incorrect and causes logic failures. Instead, we do nothing.

  // Sleep for up to timeout_ms milliseconds.
  DWORD sleep_result = ::SleepEx(timeout_ms, TRUE);

  // Timeout reached.
  if (sleep_result == 0) {
    g_waiting_for_write = false;
    ::CancelIo(file_.Get());
    return Result::kTimeout;
  }

  // Unexpected error.
  if (sleep_result != WAIT_IO_COMPLETION)
    return Result::kError;

  // AsyncWriteFinished has been called.
  if (g_error != ERROR_SUCCESS)
    return Result::kError;

  // Partial writes should not be possible.
  return g_bytes_written == size ? Result::kSuccess : Result::kError;
}

void MemlogSenderPipe::Close() {
  base::AutoLock lock(lock_);
  file_.Close();
}

}  // namespace profiling
