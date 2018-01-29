// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VIDEO_ACCELERATOR_PROTECTED_BUFFER_ALLOCATOR_H_
#define COMPONENTS_ARC_VIDEO_ACCELERATOR_PROTECTED_BUFFER_ALLOCATOR_H_

#include "base/callback_forward.h"
#include "base/threading/thread_checker.h"
#include "components/arc/common/protected_buffer_allocator.mojom.h"

namespace arc {

class ProtectedBufferManager;

// ProtectedBufferAllocator is executed in the GPU process.
// It takes allocating protected buffer request from ARC vide IPC channel.
// All the protected buffers allocated by though ProtectedBufferAllocator will
// be destructed when ProtectedBufferAllocator is destructed.
class ProtectedBufferAllocator : public mojom::ProtectedBufferAllocator {
 public:
  explicit ProtectedBufferAllocator(
      ProtectedBufferManager* protected_buffer_manager);
  ~ProtectedBufferAllocator();
  // Implementation of mojom::ProtectedBufferAllocator
  // Call ProtectedBufferManager::Create().
  // Return false only if it returns nullptr. That means, there is other
  // ProtctedBufferAllocator which is using ProtectedBufferManager.
  void AllocateProtectedBuffer(mojom::BufferType type,
                               mojom::ScopedHandle handle_fd,
                               mojom::BufferMetadata metadata,
                               AllocateProtectedBufferCallback callback);
  void DeallocateProtectedBuffer(mojom::ScopedHandle handle_fd,
                                 DeallocateProtectedBufferCallback callback);

 private:
  // Stores the shared_ptr returned by ProtectedBufferManager::Create().
  // This will never be changed once it is initialized in Initialize().
  ProtectedBufferManager* const protected_buffer_manager_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(GpuArcVideoDecodeAccelerator);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_VIDEO_ACCELERATOR_PROTECTED_BUFFER_ALLOCATOR_H_
