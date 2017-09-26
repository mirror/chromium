// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_FAKE_PROGRESS_H_
#define PDF_FAKE_PROGRESS_H_

#include <memory>

#include "base/macros.h"

namespace chrome_pdf {

class DocumentLoader;
class Timer;

class FakeProgress {
 public:
  explicit FakeProgress(DocumentLoader* loader);
  virtual ~FakeProgress();

  void Start();
  void Stop();
  void SimulateNewData();

  uint32_t GetSimulatedDocumentSize() const;
  uint32_t GetSimulatedCountOfBytesReceived() const;

 private:
  DocumentLoader* loader_;
  uint32_t simulation_call_count_ = 0;
  std::unique_ptr<Timer> timer_;

  DISALLOW_COPY_AND_ASSIGN(FakeProgress);
};

}  // namespace chrome_pdf

#endif  // PDF_FAKE_PROGRESS_H_
