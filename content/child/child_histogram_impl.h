// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_CHILD_HISTOGRAM_IMPL_H
#define CONTENT_CHILD_CHILD_HISTOGRAM_IMPL_H

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "content/common/child_histogram.mojom.h"
#include "ipc/message_filter.h"

namespace base {
class HistogramDeltaSerialization;
}  // namespace base

namespace content {

class ChildHistogramImpl : public content::mojom::ChildHistogram {
 public:
  ChildHistogramImpl();
  ~ChildHistogramImpl() override;

  static void Create(content::mojom::ChildHistogramRequest);

 private:
  typedef std::vector<std::string> HistogramPickledList;

  // content::mojom::ChildHistogram implementation.
  void SetHistogramMemory(mojo::ScopedSharedBufferHandle memory_handle,
                          int64_t memory_size) override;

  using HistogramDataCallback = content::mojom::ChildHistogram::
      GetChildNonPersistentHistogramDataCallback;

  void GetChildNonPersistentHistogramData(
      HistogramDataCallback callback) override;

  // Extract snapshot data and then send it off to the Browser process.
  // Send only a delta to what we have already sent.
  void UploadAllHistograms(int64_t sequence_number);

  // Prepares histogram deltas for transmission.
  std::unique_ptr<base::HistogramDeltaSerialization>
      histogram_delta_serialization_;

  DISALLOW_COPY_AND_ASSIGN(ChildHistogramImpl);
};

}  // namespace content

#endif  // CONTENT_CHILD_CHILD_HISTOGRAM_IMPL_H
