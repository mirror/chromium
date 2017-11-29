// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "ipc/ipc_platform_file.h"

#if defined(OS_POSIX)
#include <unistd.h>

#include "base/posix/eintr_wrapper.h"
#endif

namespace IPC {

PlatformFileForTransit::PlatformFileForTransit()
    :
#if defined(OS_WIN)
      file_(nullptr),
#endif  // defined(OS_WIN)
      is_async_(false) {
}

PlatformFileForTransit::PlatformFileForTransit(FileType file, bool is_async)
    : file_(file), is_async_(is_async) {}

bool PlatformFileForTransit::operator==(
    const PlatformFileForTransit& platform_file) const {
  // No need to check |is_async_| - if they're the same handle, that shuold be
  // the same, too.
  return file_ == platform_file.file_;
}

bool PlatformFileForTransit::operator!=(
    const PlatformFileForTransit& platform_file) const {
  return !(*this == platform_file);
}

PlatformFileForTransit::FileType PlatformFileForTransit::GetFile() const {
  return file_;
}

bool PlatformFileForTransit::IsValid() const {
#if defined(OS_WIN)
  return file_ != nullptr;
#elif defined(OS_POSIX)
  return file_.fd >= 0;
#endif
}

bool PlatformFileForTransit::IsAsync() const {
  return is_async_;
}

PlatformFileForTransit GetPlatformFileForTransit(base::PlatformFile handle,
                                                 bool close_source_handle,
                                                 bool is_async) {
#if defined(OS_WIN)
  HANDLE raw_handle = INVALID_HANDLE_VALUE;
  DWORD options = DUPLICATE_SAME_ACCESS;
  if (close_source_handle)
    options |= DUPLICATE_CLOSE_SOURCE;
  if (handle == INVALID_HANDLE_VALUE ||
      !::DuplicateHandle(::GetCurrentProcess(), handle, ::GetCurrentProcess(),
                         &raw_handle, 0, FALSE, options)) {
    return IPC::InvalidPlatformFileForTransit();
  }

  return IPC::PlatformFileForTransit(raw_handle, is_async);
#elif defined(OS_POSIX)
  // If asked to close the source, we can simply re-use the source fd instead of
  // dup()ing and close()ing.
  // When we're not closing the source, we need to duplicate the handle and take
  // ownership of that. The reason is that this function is often used to
  // generate IPC messages, and the handle must remain valid until it's sent to
  // the other process from the I/O thread. Without the dup, calling code might
  // close the source handle before the message is sent, creating a race
  // condition.
  int fd = close_source_handle ? handle : HANDLE_EINTR(::dup(handle));
  return IPC::PlatformFileForTransit(base::FileDescriptor(fd, true), is_async);
#else
  #error Not implemented.
#endif
}

PlatformFileForTransit TakePlatformFileForTransit(base::File file) {
  return GetPlatformFileForTransit(file.TakePlatformFile(), true, file.async());
}

PlatformFileForTransit DuplicatePlatformFileForTransit(const base::File& file) {
  return GetPlatformFileForTransit(file.GetPlatformFile(), false, file.async());
}

}  // namespace IPC
