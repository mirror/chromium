// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/video_decode_perf_history.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace media {

// static
void VideoDecodePerfHistory::BindRequest(
    mojom::VideoDecodePerfHistoryRequest request) {
  DVLOG(1) << __func__;

  // Single static instance should serve all requests.
  // TODO(chcunningham): Inject database
  static VideoDecodePerfHistory* instance = new VideoDecodePerfHistory();
  instance->BindRequestInternal(std::move(request));
}

VideoDecodePerfHistory::VideoDecodePerfHistory() {
  DVLOG(1) << __func__;
}

VideoDecodePerfHistory::~VideoDecodePerfHistory() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void VideoDecodePerfHistory::BindRequestInternal(
    mojom::VideoDecodePerfHistoryRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request));
}

void VideoDecodePerfHistory::GetPerfInfo(VideoCodecProfile profile,
                                         const gfx::Size& natural_size,
                                         int frames_per_sec,
                                         GetPerfInfoCallback callback) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK_NE(profile, VIDEO_CODEC_PROFILE_UNKNOWN);
  DCHECK_GT(frames_per_sec, 0);
  DCHECK(natural_size.width() > 0 && natural_size.height() > 0);

  // TODO(chcunningham): pull from database.
  bool is_smooth = false;
  bool is_power_efficient = false;

  std::move(callback).Run(is_smooth, is_power_efficient);
}

}  // namespace media