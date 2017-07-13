// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_capabilities_recorder.h"
#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

#include "base/logging.h"

namespace media {

// = default?
MediaCapabilitiesRecorder::MediaCapabilitiesRecorder()
    : frames_decoded_(0), frames_dropped_(0) {
  LOG(ERROR) << __func__ << " made one";
}
MediaCapabilitiesRecorder::~MediaCapabilitiesRecorder() {
  LOG(ERROR) << " Finalize for ipc disconnect";
  FinalizeRecord();
}

// static
void MediaCapabilitiesRecorder::Create(
    const service_manager::BindSourceInfo& source_info,
    mojom::MediaCapabilitiesRecorderRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<MediaCapabilitiesRecorder>(),
                          std::move(request));
}

void MediaCapabilitiesRecorder::StartNewRecord(VideoCodecProfile profile,
                                               const gfx::Size& natural_size,
                                               int frames_per_sec) {
  LOG(ERROR) << __func__ << "profile: " << profile
             << " sz:" << natural_size.ToString() << " fps:" << frames_per_sec;
}

void MediaCapabilitiesRecorder::UpdateRecord(int frames_decoded,
                                             int frames_dropped) {
  LOG(ERROR) << __func__ << " decoded:" << frames_decoded
             << " dropped:" << frames_dropped;

  // Should never go backwards.
  DCHECK_GE(frames_decoded, frames_decoded_);
  DCHECK_GE(frames_dropped, frames_dropped_);

  frames_decoded_ = frames_decoded;
  frames_dropped_ = frames_dropped;
}

void MediaCapabilitiesRecorder::FinalizeRecord() {
  LOG(ERROR) << __func__ << " FINAL decoded:" << frames_decoded_
             << " dropped:" << frames_dropped_;
}

}  // namespace media