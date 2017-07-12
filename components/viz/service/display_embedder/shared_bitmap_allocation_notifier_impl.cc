// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/shared_bitmap_allocation_notifier_impl.h"

#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace viz {

SharedBitmapAllocationNotifierImpl::SharedBitmapAllocationNotifierImpl(
    ServerSharedBitmapManager* manager)
    : manager_(manager), binding_(this) {}

SharedBitmapAllocationNotifierImpl::~SharedBitmapAllocationNotifierImpl() {
  ChildDied();
}

void SharedBitmapAllocationNotifierImpl::Bind(
    cc::mojom::SharedBitmapAllocationNotifierRequest request) {
  binding_.Bind(std::move(request));
}

void SharedBitmapAllocationNotifierImpl::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const SharedBitmapId& id) {
  base::SharedMemoryHandle memory_handle;
  size_t size;
  MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(buffer), &memory_handle, &size, NULL);
  DCHECK_EQ(result, MOJO_RESULT_OK);
  this->ChildAllocatedSharedBitmap(size, memory_handle, id);
  last_shared_bitmap_sequence_number_++;
  for (SharedBitmapAllocationObserver& observer : observers_)
    observer.DidAllocateSharedBitmap(last_shared_bitmap_sequence_number_);
}

void SharedBitmapAllocationNotifierImpl::DidDeleteSharedBitmap(
    const SharedBitmapId& id) {
  manager_->ChildDeletedSharedBitmap(id);
  owned_bitmaps_.erase(id);
}

void SharedBitmapAllocationNotifierImpl::ChildAllocatedSharedBitmap(
    size_t buffer_size,
    const base::SharedMemoryHandle& handle,
    const SharedBitmapId& id) {
  if (manager_->ChildAllocatedSharedBitmap(buffer_size, handle, id))
    owned_bitmaps_.insert(id);
}

void SharedBitmapAllocationNotifierImpl::ChildDied() {
  for (const auto& id : owned_bitmaps_)
    manager_->ChildDeletedSharedBitmap(id);
  owned_bitmaps_.clear();
  binding_.Close();
}

void SharedBitmapAllocationNotifierImpl::AddObserver(
    SharedBitmapAllocationObserver* observer) {
  observers_.AddObserver(observer);
  observer->DidAllocateSharedBitmap(last_shared_bitmap_sequence_number_);
}

void SharedBitmapAllocationNotifierImpl::RemoveObserver(
    SharedBitmapAllocationObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace viz
