// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/client/client_shared_bitmap_manager.h"

#include <stddef.h>

#include <utility>

#include "base/debug/alias.h"
#include "base/process/memory.h"
#include "base/process/process_metrics.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

namespace {

class ClientSharedBitmap : public SharedBitmap {
 public:
  ClientSharedBitmap(
      scoped_refptr<mojom::ThreadSafeSharedBitmapAllocationNotifierPtr>
          shared_bitmap_allocation_notifier,
      base::SharedMemory* shared_memory,
      const SharedBitmapId& id,
      uint32_t sequence_number)
      : SharedBitmap(static_cast<uint8_t*>(shared_memory->memory()),
                     id,
                     sequence_number),
        shared_bitmap_allocation_notifier_(
            std::move(shared_bitmap_allocation_notifier)) {}

  ClientSharedBitmap(
      scoped_refptr<mojom::ThreadSafeSharedBitmapAllocationNotifierPtr>
          shared_bitmap_allocation_notifier,
      std::unique_ptr<base::SharedMemory> shared_memory_holder,
      const SharedBitmapId& id,
      uint32_t sequence_number)
      : ClientSharedBitmap(std::move(shared_bitmap_allocation_notifier),
                           shared_memory_holder.get(),
                           id,
                           sequence_number) {
    shared_memory_holder_ = std::move(shared_memory_holder);
  }

  ~ClientSharedBitmap() override {
    (*shared_bitmap_allocation_notifier_)->DidDeleteSharedBitmap(id());
  }

  // SharedBitmap:
  base::SharedMemoryHandle GetSharedMemoryHandle() const override {
    if (!shared_memory_holder_)
      return base::SharedMemoryHandle();
    return shared_memory_holder_->handle();
  }

 private:
  scoped_refptr<mojom::ThreadSafeSharedBitmapAllocationNotifierPtr>
      shared_bitmap_allocation_notifier_;
  std::unique_ptr<base::SharedMemory> shared_memory_holder_;
};

// Collect extra information for debugging bitmap creation failures.
void CollectMemoryUsageAndDie(const gfx::Size& size, size_t alloc_size) {
#if defined(OS_WIN)
  int width = size.width();
  int height = size.height();
  DWORD last_error = GetLastError();

  std::unique_ptr<base::ProcessMetrics> metrics(
      base::ProcessMetrics::CreateProcessMetrics(
          base::GetCurrentProcessHandle()));

  size_t private_bytes = 0;
  size_t shared_bytes = 0;
  metrics->GetMemoryBytes(&private_bytes, &shared_bytes);

  base::debug::Alias(&width);
  base::debug::Alias(&height);
  base::debug::Alias(&last_error);
  base::debug::Alias(&private_bytes);
  base::debug::Alias(&shared_bytes);
#endif
  base::TerminateBecauseOutOfMemory(alloc_size);
}

// Allocates a block of shared memory of the given size. Returns nullptr on
// failure.
std::unique_ptr<base::SharedMemory> AllocateSharedMemory(size_t buf_size) {
  // NOTE: We use Mojo to allocate the shared buffer handle since this code may
  // run in a sandbox, and shared memory allocation may require unavailable
  // privileges. Mojo shared memory allocations are implicitly brokered as
  // needed.
  mojo::ScopedSharedBufferHandle buffer =
      mojo::SharedBufferHandle::Create(buf_size);
  if (!buffer->is_valid()) {
    LOG(WARNING) << "Browser failed to allocate shared memory";
    return nullptr;
  }

#if defined(OS_POSIX) && !defined(OS_FUCHSIA) && !defined(OS_MACOSX)
  mojo::ScopedSharedBufferHandle read_only_buffer =
      buffer->Clone(mojo::SharedBufferHandle::AccessMode::READ_ONLY);

  base::SharedMemoryHandle handle;
  if (mojo::UnwrapSharedMemoryHandle(std::move(buffer), &handle, nullptr,
                                     nullptr) != MOJO_RESULT_OK) {
    LOG(WARNING) << "Browser failed to allocate shared memory";
    return nullptr;
  }

  // TODO(rockot): We should consider introducing a mojo::UnwrapSharedMemory
  // API which can handle read-only-sharable base::SharedMemory objects properly
  // on POSIX. This would eliminate the need to unwrap the read-only handle
  // separately before reconstructing a base::SharedMemory below.
  base::SharedMemoryHandle read_only_handle;
  if (mojo::UnwrapSharedMemoryHandle(std::move(read_only_buffer),
                                     &read_only_handle, nullptr,
                                     nullptr) != MOJO_RESULT_OK) {
    LOG(WARNING) << "Browser failed to allocate shared memory";
    return nullptr;
  }

  auto memory = std::make_unique<base::SharedMemory>();
  memory->Create(handle, read_only_handle, buf_size);
#else
  base::SharedMemoryHandle handle;
  if (mojo::UnwrapSharedMemoryHandle(std::move(buffer), &handle, nullptr,
                                     nullptr) != MOJO_RESULT_OK) {
    LOG(WARNING) << "Browser failed to allocate shared memory";
    return nullptr;
  }

  auto memory = std::make_unique<base::SharedMemory>(std::move(handle), false);
#endif

  return memory;
}

}  // namespace

ClientSharedBitmapManager::ClientSharedBitmapManager(
    scoped_refptr<mojom::ThreadSafeSharedBitmapAllocationNotifierPtr>
        shared_bitmap_allocation_notifier)
    : shared_bitmap_allocation_notifier_(
          std::move(shared_bitmap_allocation_notifier)) {}

ClientSharedBitmapManager::~ClientSharedBitmapManager() {}

std::unique_ptr<SharedBitmap> ClientSharedBitmapManager::AllocateSharedBitmap(
    const gfx::Size& size) {
  TRACE_EVENT2("renderer", "ClientSharedBitmapManager::AllocateSharedBitmap",
               "width", size.width(), "height", size.height());
  size_t memory_size;
  if (!SharedBitmap::SizeInBytes(size, &memory_size))
    return nullptr;
  SharedBitmapId id = SharedBitmap::GenerateId();
  std::unique_ptr<base::SharedMemory> memory =
      AllocateSharedMemory(memory_size);
  if (!memory || !memory->Map(memory_size))
    CollectMemoryUsageAndDie(size, memory_size);

  uint32_t sequence_number = NotifyAllocatedSharedBitmap(memory.get(), id);

  // Close the associated FD to save resources, the previously mapped memory
  // remains available.
  memory->Close();

  return std::make_unique<ClientSharedBitmap>(
      shared_bitmap_allocation_notifier_, std::move(memory), id,
      sequence_number);
}

std::unique_ptr<SharedBitmap> ClientSharedBitmapManager::GetSharedBitmapFromId(
    const gfx::Size&,
    const SharedBitmapId&) {
  NOTREACHED();
  return nullptr;
}

std::unique_ptr<SharedBitmap>
ClientSharedBitmapManager::GetBitmapForSharedMemory(base::SharedMemory* mem) {
  SharedBitmapId id = SharedBitmap::GenerateId();
  uint32_t sequence_number = NotifyAllocatedSharedBitmap(mem, id);
  return std::make_unique<ClientSharedBitmap>(
      shared_bitmap_allocation_notifier_, mem, id, sequence_number);
}

// Notifies the browser process that a shared bitmap with the given ID was
// allocated. Caller keeps ownership of |memory|.
uint32_t ClientSharedBitmapManager::NotifyAllocatedSharedBitmap(
    base::SharedMemory* memory,
    const SharedBitmapId& id) {
  base::SharedMemoryHandle handle_to_send = memory->GetReadOnlyHandle();
  if (!base::SharedMemory::IsHandleValid(handle_to_send)) {
    LOG(ERROR) << "Failed to duplicate shared memory handle for bitmap.";
    return 0;
  }

  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      handle_to_send, memory->mapped_size(),
      mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);

  {
    base::AutoLock lock(lock_);
    (*shared_bitmap_allocation_notifier_)
        ->DidAllocateSharedBitmap(std::move(buffer_handle), id);
    return ++last_sequence_number_;
  }
}

}  // namespace viz
