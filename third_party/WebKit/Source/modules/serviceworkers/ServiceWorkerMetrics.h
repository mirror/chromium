// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstddef>
#include "platform/wtf/dtoa/utils.h"

namespace blink {

class ServiceWorkerMetrics {
 public:
  // Records the size of the worker's main script.
  static void RecordScriptSize(size_t bytes);
  // Records the size of the V8 code cache for the main script.
  static void RecordCachedMetadataSize(size_t bytes);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ServiceWorkerMetrics);
};

}  // namespace blink
