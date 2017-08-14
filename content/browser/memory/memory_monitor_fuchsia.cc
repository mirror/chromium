// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor.h"

#include "base/logging.h"

namespace content {

std::unique_ptr<MemoryMonitor> CreateMemoryMonitor() {
  // TODO(fuchsia): Implement this. (crbug.com/750934)
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace content
