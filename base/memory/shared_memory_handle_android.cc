// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory_handle.h"

#include <unistd.h>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket.h"
#include "base/unguessable_token.h"

#include "ui/gfx/android/android_hardware_buffer_compat.h"

namespace base {

SharedMemoryHandle::SharedMemoryHandle() {
  LOG(INFO) << __FUNCTION__ << ";;; type=" << type_ << " memory_object_=" << memory_object_;
}

SharedMemoryHandle::SharedMemoryHandle(
    const base::FileDescriptor& file_descriptor,
    size_t size,
    const base::UnguessableToken& guid)
    : type_(POSIX),
      file_descriptor_(file_descriptor),
      guid_(guid),
      size_(size) {
  LOG(INFO) << __FUNCTION__ << ";;; type=" << type_ << " memory_object_=" << memory_object_;
}

SharedMemoryHandle::SharedMemoryHandle(
    const base::FileDescriptor& file_descriptor,
    const base::UnguessableToken& guid)
    : type_(ANDROID_HARDWARE_BUFFER),
      guid_(guid),
      size_(0) {
  LOG(INFO) << __FUNCTION__ << ";;; type=" << type_;

  if (file_descriptor.fd < 0) {
    LOG(INFO) << __FUNCTION__ << ";;; invalid fd=" << file_descriptor.fd;
    type_ = POSIX;
    file_descriptor_ = file_descriptor;
    return;
  }
  gfx::AndroidHardwareBufferCompat::EnsureFunctionsLoaded();

  AHardwareBuffer* ahb = nullptr;

  int ret = AHardwareBuffer_recvHandleFromUnixSocket(file_descriptor.fd, &ahb);

  if (ret < 0) {
    LOG(ERROR) << __FUNCTION__ << ";;; recv failed! FIXME!";
    type_ = POSIX;
    file_descriptor_ = file_descriptor;
    return;
  }

  // Do *not* call AHardwareBuffer_acquire(hbp), recvHandleFromUnixSocket did
  // so already. We do need to call release on it when we're done, i.e. after
  // binding it to an image. That happens in the destructor. We can't
  // preemptively release it here, that invalidates our accessor pointer.
  //
  // We don't need to use AHardwareBuffer_lock, that's just for accessing
  // memory from CPU which we're not currently doing.
  memory_object_ = ahb;

  LOG(INFO) << __FUNCTION__ << ";;; type=" << type_ << " memory_object_=" << memory_object_;
}

SharedMemoryHandle::SharedMemoryHandle(struct AHardwareBuffer* buffer,
                                       size_t size,
                                       const base::UnguessableToken& guid)
    : type_(ANDROID_HARDWARE_BUFFER),
      memory_object_(buffer),
      ownership_passes_to_ipc_(false),
      guid_(guid),
      size_(size) {
  LOG(INFO) << __FUNCTION__ << ";;; type=" << type_ << " memory_object_=" << memory_object_;
}

// static
SharedMemoryHandle SharedMemoryHandle::ImportHandle(int fd, size_t size) {
  SharedMemoryHandle handle;
  handle.type_ = POSIX;
  handle.file_descriptor_.fd = fd;
  handle.file_descriptor_.auto_close = false;
  handle.guid_ = UnguessableToken::Create();
  handle.size_ = size;
  LOG(INFO) << __FUNCTION__ << ";;; type=" << handle.type_ << " memory_object_=" << handle.memory_object_;
  return handle;
}

int SharedMemoryHandle::GetHandle() const {
  LOG(INFO) << __FUNCTION__ << ";;; type=" << type_ << " memory_object_=" << memory_object_;
  switch (type_) {
    case POSIX:
      return file_descriptor_.fd;
    case ANDROID_HARDWARE_BUFFER:
      if (!IsValid()) return -1;

      //EnsureFunctionsLoaded();

      base::ScopedFD read_fd, write_fd;
      base::CreateSocketPair(&read_fd, &write_fd);

      int ret = AHardwareBuffer_sendHandleToUnixSocket(
          memory_object_, write_fd.get());

      if (ret < 0) {
        LOG(ERROR) << __FUNCTION__ << ";;; send failed! FIXME!";
        return -1;
      }

      // Close now to avoid timeouts in case receiver goes away? cf. elsewhere.
      write_fd.reset();

      LOG(INFO) << __FUNCTION__ << ";;; read_fd=" << read_fd.get();

      return read_fd.release();
  }
}

bool SharedMemoryHandle::IsValid() const {
  switch (type_) {
    case POSIX:
      return file_descriptor_.fd >= 0;
    case ANDROID_HARDWARE_BUFFER:
      return memory_object_ != nullptr;
  }
}

void SharedMemoryHandle::Close() const {
  switch (type_) {
    case POSIX:
      if (IGNORE_EINTR(close(file_descriptor_.fd)) < 0)
        PLOG(ERROR) << "close";
      break;
    case ANDROID_HARDWARE_BUFFER:
      if (!IsValid())
        return;
      LOG(INFO) << __FUNCTION__ << ";;; memory_object_=" << memory_object_;
      AHardwareBuffer_release(memory_object_);
  }
}

int SharedMemoryHandle::Release() {
  DCHECK_EQ(type_, POSIX);
  int old_fd = file_descriptor_.fd;
  file_descriptor_.fd = -1;
  return old_fd;
}

SharedMemoryHandle SharedMemoryHandle::Duplicate() const {
  switch (type_) {
    case POSIX: {
      if (!IsValid())
        return SharedMemoryHandle();

      int duped_handle = HANDLE_EINTR(dup(file_descriptor_.fd));
      if (duped_handle < 0)
        return SharedMemoryHandle();
      return SharedMemoryHandle(FileDescriptor(duped_handle, true), GetSize(),
                                GetGUID());
    }
    case ANDROID_HARDWARE_BUFFER:
      if (!IsValid())
        return SharedMemoryHandle();
      LOG(INFO) << __FUNCTION__ << ";;; memory_object_=" << memory_object_;
      AHardwareBuffer_acquire(memory_object_);
      SharedMemoryHandle handle(*this);
      handle.SetOwnershipPassesToIPC(true);
      LOG(INFO) << __FUNCTION__ << ";;; COPY type=" << handle.type_ << " memory_object_=" << handle.memory_object_;
      return handle;
  }
}

struct AHardwareBuffer* SharedMemoryHandle::GetMemoryObject() const {
  DCHECK_EQ(type_, ANDROID_HARDWARE_BUFFER);
  LOG(INFO) << __FUNCTION__ << ";;; memory_object_=" << memory_object_;
  return memory_object_;
}

void SharedMemoryHandle::SetOwnershipPassesToIPC(bool ownership_passes) {
  switch (type_) {
    case POSIX:
      file_descriptor_.auto_close = ownership_passes;
      break;
    case ANDROID_HARDWARE_BUFFER:
      ownership_passes_to_ipc_ = ownership_passes;
  }
}

bool SharedMemoryHandle::OwnershipPassesToIPC() const {
  switch (type_) {
    case POSIX:
      return file_descriptor_.auto_close;
    case ANDROID_HARDWARE_BUFFER:
      return ownership_passes_to_ipc_;
  }
}

}  // namespace base
