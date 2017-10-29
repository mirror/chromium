// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/mojo_ukm_recorder.h"

#include <utility>

namespace ukm {

MojoUkmRecorder::MojoUkmRecorder(mojom::UkmRecorderInterfacePtr interface)
    : interface_(std::move(interface)), weak_factory_(this) {}
MojoUkmRecorder::~MojoUkmRecorder() = default;

void MojoUkmRecorder::UpdateSourceURL(SourceId source_id, const GURL& url) {
  interface_->UpdateSourceURL(source_id, url.spec());
}

base::WeakPtr<MojoUkmRecorder> MojoUkmRecorder::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void MojoUkmRecorder::AddEntry(mojom::UkmEntryPtr entry) {
  LOG(ERROR) << "MojoUkmRecorder::AddEntry";
  interface_->AddEntry(std::move(entry));
}

}  // namespace ukm
