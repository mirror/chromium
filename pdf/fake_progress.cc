// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/fake_progress.h"

#include "base/memory/ptr_util.h"
#include "pdf/document_loader.h"
#include "pdf/timer.h"

namespace {

constexpr int kFakeNewDataDelayMs = 400;
constexpr uint32_t kFakeNewDataPartSize = 1024;
constexpr uint32_t kMaxSimulationCallCount =
    (5 * 60 * 1000) / kFakeNewDataDelayMs;  // 5 min

class NewFakeDataTimer : public chrome_pdf::Timer {
 public:
  explicit NewFakeDataTimer(chrome_pdf::FakeProgress* owner)
      : Timer(kFakeNewDataDelayMs), owner_(owner) {}

  ~NewFakeDataTimer() override {}

  void OnTimer() override { owner_->SimulateNewData(); }

 private:
  chrome_pdf::FakeProgress* owner_;

  DISALLOW_COPY_AND_ASSIGN(NewFakeDataTimer);
};

}  // namespace

namespace chrome_pdf {

FakeProgress::FakeProgress(DocumentLoader* loader) : loader_(loader) {}

FakeProgress::~FakeProgress() {}

void FakeProgress::SimulateNewData() {
  if (++simulation_call_count_ == kMaxSimulationCallCount) {
    Stop();
    return;
  }
  loader_->client()->OnNewDataReceived();
}

void FakeProgress::Start() {
  timer_ = base::MakeUnique<NewFakeDataTimer>(this);
  simulation_call_count_ = 0;
}

void FakeProgress::Stop() {
  timer_.reset();
}

uint32_t FakeProgress::GetSimulatedDocumentSize() const {
  return loader_->GetDocumentSize();
}

uint32_t FakeProgress::GetSimulatedCountOfBytesReceived() const {
  const uint32_t document_size = loader_->GetDocumentSize();
  if (!document_size)
    return loader_->count_of_bytes_received() +
           simulation_call_count_ * kFakeNewDataPartSize;

  const uint32_t real_received = loader_->count_of_bytes_received();
  if (simulation_call_count_ == 0 || real_received >= document_size)
    return real_received;

  const uint32_t not_received = document_size - real_received;

  const double simulated_progress = sqrt(
      static_cast<double>(simulation_call_count_) / kMaxSimulationCallCount);
  const uint32_t simulated_received = not_received * 0.5 * simulated_progress;
  return real_received + simulated_received;
}

}  // namespace chrome_pdf
