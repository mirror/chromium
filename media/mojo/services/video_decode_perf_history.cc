// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/video_decode_perf_history.h"

#include "base/callback.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "media/base/video_codecs.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace media {

VideoDecodeStatsDBFactory* g_db_factory = nullptr;

// static
void VideoDecodePerfHistory::SetDatabaseFactory(
    VideoDecodeStatsDBFactory* db_factory) {
  DVLOG(2) << __func__;
  g_db_factory = db_factory;
}

// static
VideoDecodePerfHistory* VideoDecodePerfHistory::GetSingletonInstance() {
  static VideoDecodePerfHistory* instance = new VideoDecodePerfHistory();
  return instance;
}

// static
void VideoDecodePerfHistory::BindRequest(
    mojom::VideoDecodePerfHistoryRequest request) {
  DVLOG(2) << __func__;

  // Single static instance should serve all requests.
  GetSingletonInstance()->BindRequestInternal(std::move(request));
}

VideoDecodePerfHistory::VideoDecodePerfHistory()
    : db_(g_db_factory->CreateDB()) {
  // FIXME track initialize, defer saves/queries until its ready.
  db_->Initialize(base::BindOnce(&VideoDecodePerfHistory::OnDatabseInit,
                                 base::Unretained(this)));

  DVLOG(2) << __func__;
}

VideoDecodePerfHistory::~VideoDecodePerfHistory() {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void VideoDecodePerfHistory::BindRequestInternal(
    mojom::VideoDecodePerfHistoryRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request));
}

void VideoDecodePerfHistory::OnDatabseInit(bool success) {
  DVLOG(2) << __func__ << " " << success;
}

void VideoDecodePerfHistory::OnGotStatsForRequest(
    const VideoDecodeStatsDB::VideoDescKey& video_key,
    GetPerfInfoCallback mojo_cb,
    bool database_success,
    std::unique_ptr<VideoDecodeStatsDB::DecodeStatsEntry> stats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!mojo_cb.is_null());

  bool is_power_efficient;
  bool is_smooth;
  double percent_dropped = 0;

  if (stats.get()) {
    DCHECK(database_success);
    percent_dropped =
        static_cast<double>(stats->frames_dropped) / stats->frames_decoded;

    // TODO(chcunningham): add statistics for power efficiency to database.
    is_power_efficient = true;
    is_smooth = percent_dropped <= kMaxSmoothDroppedFramesPercent;
  } else {
    // TODO(chcunningham/mlamouri): Refactor database API to give us nearby
    // stats whenever we don't have a perfect match. If higher
    // resolutions/framerates are known to be smooth, we can report this as
    /// smooth. If lower resolutions/frames are known to be janky, we can assume
    // this will be janky.

    // No stats? Lets be optimistic.
    is_power_efficient = true;
    is_smooth = true;
  }

  DVLOG(3) << __func__
           << base::StringPrintf(
                  " profile:%s size:%s fps:%d --> ",
                  GetProfileName(video_key.codec_profile).c_str(),
                  video_key.size.ToString().c_str(), video_key.frame_rate)
           << (stats.get()
                   ? base::StringPrintf(
                         "smooth:%d frames_decoded:%" PRIu64 " pcnt_dropped:%f",
                         is_smooth, stats->frames_decoded, percent_dropped)
                   : (database_success ? "no info" : "query FAILED"));

  std::move(mojo_cb).Run(is_smooth, is_power_efficient);
}

void VideoDecodePerfHistory::GetPerfInfo(VideoCodecProfile profile,
                                         const gfx::Size& natural_size,
                                         int frame_rate,
                                         GetPerfInfoCallback callback) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK_NE(profile, VIDEO_CODEC_PROFILE_UNKNOWN);
  DCHECK_GT(frame_rate, 0);
  DCHECK(natural_size.width() > 0 && natural_size.height() > 0);

  VideoDecodeStatsDB::VideoDescKey video_key(profile, natural_size, frame_rate);

  // Unretained is safe because this is a leaky singleton.
  db_->GetDecodeStats(
      video_key,
      base::BindOnce(&VideoDecodePerfHistory::OnGotStatsForRequest,
                     base::Unretained(this), video_key, std::move(callback)));
}

void VideoDecodePerfHistory::ReportUkmMetrics(
    const VideoDecodeStatsDB::VideoDescKey& video_key,
    const VideoDecodeStatsDB::DecodeStatsEntry& new_stats,
    VideoDecodeStatsDB::DecodeStatsEntry* past_stats) {
  /*
  basic stream description: codec profile, resolution, framerate
  media capabilities expected performance
  booleans indicating hypothetical claims of is_smooth or is_power_efficient
  historical performance stats - total counts for frames decoded, frames
  dropped, frames power efficient media capabilities grade of observed
  performance booleans for does *this* playback qualify as smooth or power
  efficient? current performance stats - counts from this playback of frames
  decoded, frames dropped, frames power efficient
  */

  // UKM may be unavailable in content_shell or other non-chrome/ builds; it
  // may also be unavailable if browser shutdown has started; so this may be a
  // nullptr. If it's unavailable, UKM reporting will be skipped.
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  // FIXME - plumb URL
  // ukm_recorder->UpdateSourceURL(source_id, properties_->origin.GetURL());
}

void VideoDecodePerfHistory::OnGotStatsForSave(
    const VideoDecodeStatsDB::VideoDescKey& video_key,
    const VideoDecodeStatsDB::DecodeStatsEntry& new_stats,
    bool success,
    std::unique_ptr<VideoDecodeStatsDB::DecodeStatsEntry> past_stats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!success) {
    // FIXME LOG ERROR
    return;
  }

  ReportUkmMetrics(video_key, new_stats, past_stats.get());

  db_->AppendDecodeStats(video_key, new_stats);
}

void VideoDecodePerfHistory::SavePerfRecord(VideoCodecProfile profile,
                                            const gfx::Size& natural_size,
                                            int frames_per_sec,
                                            uint32_t frames_decoded,
                                            uint32_t frames_dropped) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VideoDecodeStatsDB::VideoDescKey video_key(profile, natural_size,
                                             frames_per_sec);
  VideoDecodeStatsDB::DecodeStatsEntry new_stats(frames_decoded,
                                                 frames_dropped);

  // Get past perf info and report UKM metrics before saving this record.
  db_->GetDecodeStats(
      video_key, base::BindOnce(&VideoDecodePerfHistory::OnGotStatsForSave,
                                base::Unretained(this), video_key, new_stats));
}

}  // namespace media
