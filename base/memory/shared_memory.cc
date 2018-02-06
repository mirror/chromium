// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory.h"

namespace base {

void* SharedMemory::TakeMemory() {
  void* memory = memory_;
  memory_ = nullptr;
  mapped_size_ = 0;
  mapped_id_ = UnguessableToken();
  return memory;
}

};  // namespace base
