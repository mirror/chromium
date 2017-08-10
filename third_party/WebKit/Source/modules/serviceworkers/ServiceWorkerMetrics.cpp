// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/serviceworkers/ServiceWorkerMetrics.h"

#include "platform/Histogram.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

// static
void ServiceWorkerMetrics::RecordScriptSize(size_t bytes) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      CustomCountHistogram, script_size_histogram,
      ("ServiceWorker.ScriptSize", 1000, 5000000, 50));
  script_size_histogram.Count(bytes);
}

// static
void ServiceWorkerMetrics::RecordCachedMetadataSize(size_t bytes) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      CustomCountHistogram, script_cached_metadata_size_histogram,
      ("ServiceWorker.ScriptCachedMetadataSize", 1000, 50000000, 50));
  script_cached_metadata_size_histogram.Count(bytes);
}

}  // namespace blink
