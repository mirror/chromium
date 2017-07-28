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
    : frames_decoded_(0), frames_dropped_(0) {}
MediaCapabilitiesRecorder::~MediaCapabilitiesRecorder() {
  DVLOG(2) << __func__ << " Finalize for ipc disconnect";
  FinalizeRecord();
}

// static
void MediaCapabilitiesRecorder::Create(
    mojom::MediaCapabilitiesRecorderRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<MediaCapabilitiesRecorder>(),
                          std::move(request));
}

void MediaCapabilitiesRecorder::StartNewRecord(VideoCodecProfile profile,
                                               const gfx::Size& natural_size,
                                               int frames_per_sec) {
  DCHECK_NE(profile, VIDEO_CODEC_PROFILE_UNKNOWN);
  DCHECK_GT(frames_per_sec, 0);
  DCHECK(natural_size.width() > 0 && natural_size.height() > 0);

  FinalizeRecord();

  DVLOG(2) << __func__ << "profile: " << profile
           << " sz:" << natural_size.ToString() << " fps:" << frames_per_sec;

  profile_ = profile;
  natural_size_ = natural_size;
  frames_per_sec_ = frames_per_sec;
  frames_decoded_ = 0;
  frames_dropped_ = 0;
}

void MediaCapabilitiesRecorder::UpdateRecord(int frames_decoded,
                                             int frames_dropped) {
  DVLOG(3) << __func__ << " decoded:" << frames_decoded
           << " dropped:" << frames_dropped;

  // Dropped can never exceed decoded.
  DCHECK_LE(frames_dropped, frames_decoded);
  // Should never go backwards.
  DCHECK_GE(frames_decoded, frames_decoded_);
  DCHECK_GE(frames_dropped, frames_dropped_);

  frames_decoded_ = frames_decoded;
  frames_dropped_ = frames_dropped;
}

void MediaCapabilitiesRecorder::FinalizeRecord() {
  if (profile_ == VIDEO_CODEC_PROFILE_UNKNOWN || frames_decoded_ == 0)
    return;

  DVLOG(2) << __func__ << " profile: " << profile_
           << " size:" << natural_size_.ToString() << " fps:" << frames_per_sec_
           << " decoded:" << frames_decoded_ << " dropped:" << frames_dropped_;

  // TODO(chcunningham): Save everything to DB.
}

}  // namespace media