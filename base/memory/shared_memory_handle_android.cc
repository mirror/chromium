// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory_handle.h"

#include <unistd.h>

#include "base/android/android_hardware_buffer_compat.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket.h"
#include "base/unguessable_token.h"

namespace base {

SharedMemoryHandle::SharedMemoryHandle() {}

SharedMemoryHandle::SharedMemoryHandle(
    const base::FileDescriptor& file_descriptor,
    size_t size,
    const base::UnguessableToken& guid)
    : SharedMemoryHandle(file_descriptor, size, guid, Type::POSIX) {}

SharedMemoryHandle::SharedMemoryHandle(
    const base::FileDescriptor& file_descriptor,
    size_t size,
    const base::UnguessableToken& guid,
    Type type)
    : type_(type), guid_(guid), size_(size) {
  switch (type) {
    case Type::NO_HANDLE:
      NOTREACHED() << "Can't create a Type::NO_HANDLE from a file descriptor";
      break;
    case Type::POSIX:
      DCHECK_GE(file_descriptor.fd, 0);
      file_descriptor_ = file_descriptor;
      break;
    case Type::ANDROID_HARDWARE_BUFFER:
      // This may be the first use of AHardwareBuffers in this process, so we
      // need to load symbols. This should not fail since we're supposedly
      // receiving one from IPC, but better to be paranoid.
      if (!base::AndroidHardwareBufferCompat::IsSupportAvailable()) {
        NOTREACHED() << "AHardwareBuffer support not available";
        type_ = Type::NO_HANDLE;
        return;
      }

      AHardwareBuffer* ahb = nullptr;
      // A successful receive increments refcount, we don't need to do so
      // separately.
      int ret =
          AHardwareBuffer_recvHandleFromUnixSocket(file_descriptor.fd, &ahb);

      if (ret < 0) {
        PLOG(ERROR) << "recv";
        type_ = Type::NO_HANDLE;
        return;
      }

      memory_object_ = ahb;
  }
}

SharedMemoryHandle::SharedMemoryHandle(struct AHardwareBuffer* buffer,
                                       size_t size,
                                       const base::UnguessableToken& guid)
    : type_(Type::ANDROID_HARDWARE_BUFFER),
      memory_object_(buffer),
      ownership_passes_to_ipc_(false),
      guid_(guid),
      size_(size) {}

// static
SharedMemoryHandle SharedMemoryHandle::ImportHandle(int fd, size_t size) {
  SharedMemoryHandle handle;
  handle.type_ = Type::POSIX;
  handle.file_descriptor_.fd = fd;
  handle.file_descriptor_.auto_close = false;
  handle.guid_ = UnguessableToken::Create();
  handle.size_ = size;
  return handle;
}

int SharedMemoryHandle::GetHandle() const {
  switch (type_) {
    case Type::NO_HANDLE:
      return -1;
    case Type::POSIX:
      DCHECK(IsValid());
      return file_descriptor_.fd;
    case Type::ANDROID_HARDWARE_BUFFER:
      DCHECK(IsValid());
      base::ScopedFD read_fd, write_fd;
      base::CreateSocketPair(&read_fd, &write_fd);

      int ret = AHardwareBuffer_sendHandleToUnixSocket(memory_object_,
                                                       write_fd.get());

      if (ret < 0) {
        PLOG(ERROR) << "send";
        return -1;
      }

      // Close write end now to avoid timeouts in case the receiver goes away.
      write_fd.reset();

      return read_fd.release();
  }
}

bool SharedMemoryHandle::IsValid() const {
  return type_ != Type::NO_HANDLE;
}

void SharedMemoryHandle::Close() const {
  switch (type_) {
    case Type::NO_HANDLE:
      return;
    case Type::POSIX:
      DCHECK(IsValid());
      if (IGNORE_EINTR(close(file_descriptor_.fd)) < 0)
        PLOG(ERROR) << "close";
      break;
    case Type::ANDROID_HARDWARE_BUFFER:
      DCHECK(IsValid());
      AHardwareBuffer_release(memory_object_);
  }
}

int SharedMemoryHandle::Release() {
  DCHECK_EQ(type_, Type::POSIX);
  int old_fd = file_descriptor_.fd;
  file_descriptor_.fd = -1;
  return old_fd;
}

SharedMemoryHandle SharedMemoryHandle::Duplicate() const {
  switch (type_) {
    case Type::NO_HANDLE:
      return SharedMemoryHandle();
    case Type::POSIX: {
      DCHECK(IsValid());
      int duped_handle = HANDLE_EINTR(dup(file_descriptor_.fd));
      if (duped_handle < 0)
        return SharedMemoryHandle();
      return SharedMemoryHandle(FileDescriptor(duped_handle, true), GetSize(),
                                GetGUID());
    }
    case Type::ANDROID_HARDWARE_BUFFER:
      DCHECK(IsValid());
      AHardwareBuffer_acquire(memory_object_);
      SharedMemoryHandle handle(*this);
      handle.SetOwnershipPassesToIPC(true);
      return handle;
  }
}

struct AHardwareBuffer* SharedMemoryHandle::GetMemoryObject() const {
  DCHECK_EQ(type_, Type::ANDROID_HARDWARE_BUFFER);
  return memory_object_;
}

void SharedMemoryHandle::SetOwnershipPassesToIPC(bool ownership_passes) {
  switch (type_) {
    case Type::POSIX:
      file_descriptor_.auto_close = ownership_passes;
      break;
    case Type::NO_HANDLE:
    case Type::ANDROID_HARDWARE_BUFFER:
      ownership_passes_to_ipc_ = ownership_passes;
  }
}

bool SharedMemoryHandle::OwnershipPassesToIPC() const {
  switch (type_) {
    case Type::POSIX:
      return file_descriptor_.auto_close;
    case Type::NO_HANDLE:
    case Type::ANDROID_HARDWARE_BUFFER:
      return ownership_passes_to_ipc_;
  }
}

}  // namespace base
