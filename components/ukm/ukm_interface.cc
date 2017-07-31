// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/ukm_interface.h"

#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "url/gurl.h"

namespace ukm {

UkmInterface::UkmInterface(UkmRecorder* ukm_recorder)
    : ukm_recorder_(ukm_recorder) {}

UkmInterface::~UkmInterface() = default;

// static
void UkmInterface::Create(UkmRecorder* ukm_recorder,
                          mojom::UkmRecorderInterfaceRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<UkmInterface>(ukm_recorder),
                          std::move(request));
}

void UkmInterface::AddEntry(mojom::UkmEntryPtr ukm_entry) {
  ukm_recorder_->AddEntry(std::move(ukm_entry));
}

void UkmInterface::UpdateSourceURL(int64_t source_id, const std::string& url) {
  ukm_recorder_->UpdateSourceURL(source_id, GURL(url));
}

}  // namespace ukm
