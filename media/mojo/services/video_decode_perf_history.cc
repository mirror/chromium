// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "media/base/video_codecs.h"
#include "media/mojo/services/video_decode_perf_history.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace media {

static base::OnceCallback<std::unique_ptr<MediaCapabilitiesDatabase>()>
    g_testing_db_provider_cb;

// static
void VideoDecodePerfHistory::BindRequest(
    mojom::VideoDecodePerfHistoryRequest request) {
  DVLOG(1) << __func__;

  // Single static instance should serve all requests.
  // TODO(chcunningham): Wire up to the real DB. Not sure if we want to take
  // ownership.
  static VideoDecodePerfHistory* instance = new VideoDecodePerfHistory(nullptr);
  instance->BindRequestInternal(std::move(request));
}

VideoDecodePerfHistory::VideoDecodePerfHistory(
    std::unique_ptr<MediaCapabilitiesDatabase> database)
    : database_(std::move(database)) {
  DVLOG(1) << __func__;

  if (!database_.get()) {
    LOG(ERROR) << __func__ << " No DB!";
  }
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

void VideoDecodePerfHistory::OnGotPerfInfo(
    MediaCapabilitiesDatabase::Entry entry,
    GetPerfInfoCallback mojo_cb,
    bool database_success,
    std::unique_ptr<MediaCapabilitiesDatabase::Info> info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!mojo_cb.is_null());

  std::stringstream ss;
  ss << " profile:" << GetProfileName(entry.codec_profile())
     << " size:" << entry.size().ToString() << " fps:" << entry.frame_rate()
     << " --> ";

  bool is_power_efficient;
  bool is_smooth;

  if (info.get()) {
    DCHECK(database_success);
    double percent_dropped =
        static_cast<double>(info->frames_dropped) / info->frames_decoded;
    ss << " frames_decoded: " << info->frames_decoded
       << " pcnt_dropped: " << percent_dropped;

    // TODO(chcunningham): add statistics for power efficiency to database.
    is_power_efficient = true;
    is_smooth = percent_dropped <= kMaxSmoothDroppedFramesPercent;
  } else {
    if (database_success)
      ss << " no info";
    else
      ss << " query FAILED";

    // TODO(chcunningham/mlamouri): Refactor database API to give us nearby
    // entry info whenever we don't have a perfect match. If higher
    // resolutions/framerates are known to be smooth, we can report this as
    // smooth. If lower resolutions/frames are known to be janky, we can assume
    // this will be janky.

    // No entry? Lets be optimistic.
    is_power_efficient = true;
    is_smooth = true;
  }

  DVLOG(3) << __func__ << ss.str();
  std::move(mojo_cb).Run(is_smooth, is_power_efficient);
}

void VideoDecodePerfHistory::GetPerfInfo(VideoCodecProfile profile,
                                         const gfx::Size& natural_size,
                                         int frame_rate,
                                         GetPerfInfoCallback callback) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(database_.get());

  DCHECK_NE(profile, VIDEO_CODEC_PROFILE_UNKNOWN);
  DCHECK_GT(frame_rate, 0);
  DCHECK(natural_size.width() > 0 && natural_size.height() > 0);

  // Unretained is safe because this is a leaky singleton.
  MediaCapabilitiesDatabase::Entry db_entry(profile, natural_size, frame_rate);
  database_->GetInfo(db_entry,
                     base::BindOnce(&VideoDecodePerfHistory::OnGotPerfInfo,
                                    base::Unretained(this), db_entry,
                                    base::Passed(std::move(callback))));
}

}  // namespace media