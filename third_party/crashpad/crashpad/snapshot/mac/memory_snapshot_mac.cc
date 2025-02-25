// Copyright 2014 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "snapshot/mac/memory_snapshot_mac.h"

#include <memory>

#include "util/mach/task_memory.h"

namespace crashpad {
namespace internal {

MemorySnapshotMac::MemorySnapshotMac()
    : MemorySnapshot(),
      process_reader_(nullptr),
      address_(0),
      size_(0),
      initialized_() {
}

MemorySnapshotMac::~MemorySnapshotMac() {
}

void MemorySnapshotMac::Initialize(ProcessReader* process_reader,
                                   uint64_t address,
                                   uint64_t size) {
  INITIALIZATION_STATE_SET_INITIALIZING(initialized_);
  process_reader_ = process_reader;
  address_ = address;
  size_ = size;
  INITIALIZATION_STATE_SET_VALID(initialized_);
}

uint64_t MemorySnapshotMac::Address() const {
  INITIALIZATION_STATE_DCHECK_VALID(initialized_);
  return address_;
}

size_t MemorySnapshotMac::Size() const {
  INITIALIZATION_STATE_DCHECK_VALID(initialized_);
  return size_;
}

bool MemorySnapshotMac::Read(Delegate* delegate) const {
  INITIALIZATION_STATE_DCHECK_VALID(initialized_);

  if (size_ == 0) {
    return delegate->MemorySnapshotDelegateRead(nullptr, size_);
  }

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size_]);
  if (!process_reader_->Memory()->Read(address_, size_, buffer.get())) {
    return false;
  }
  return delegate->MemorySnapshotDelegateRead(buffer.get(), size_);
}

const MemorySnapshot* MemorySnapshotMac::MergeWithOtherSnapshot(
    const MemorySnapshot* other) const {
  return MergeWithOtherSnapshotImpl(this, other);
}

}  // namespace internal
}  // namespace crashpad
