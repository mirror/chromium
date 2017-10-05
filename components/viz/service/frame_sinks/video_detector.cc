// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/video_detector.h"

#include "base/time/time.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/rect.h"

namespace viz {

const int kMinUpdateWidth = 333;
const int kMinUpdateHeight = 250;
const int kMinFramesPerSecond = 15;
const int kVideoTimeoutMs = 1000;
const int kMinVideoDurationMs = 3000;

// Stores information about updates to a window and determines whether it's
// likely that a video is playing in it.
class VideoDetector::ClientInfo {
 public:
  ClientInfo() : buffer_start_(0), buffer_size_(0) {}

  // Handles an update within a window, returning true if it appears that
  // video is currently playing in the window.
  bool RecordUpdateAndCheckForVideo(const gfx::Rect& region,
                                    base::TimeTicks now) {
    if (region.width() < kMinUpdateWidth || region.height() < kMinUpdateHeight)
      return false;

    // If the buffer is full, drop the first timestamp.
    if (buffer_size_ == static_cast<size_t>(kMinFramesPerSecond)) {
      buffer_start_ = (buffer_start_ + 1) % kMinFramesPerSecond;
      buffer_size_--;
    }

    update_times_[(buffer_start_ + buffer_size_) % kMinFramesPerSecond] = now;
    buffer_size_++;

    const bool in_video =
        (buffer_size_ == static_cast<size_t>(kMinFramesPerSecond)) &&
        ((now - update_times_[buffer_start_]).InSecondsF() <= 1.0);

    if (in_video && video_start_time_.is_null())
      video_start_time_ = update_times_[buffer_start_];
    else if (!in_video && !video_start_time_.is_null())
      video_start_time_ = base::TimeTicks();

    const base::TimeDelta elapsed = now - video_start_time_;
    return in_video &&
           elapsed >= base::TimeDelta::FromMilliseconds(kMinVideoDurationMs);
  }

 private:
  // Circular buffer containing update times of the last (up to
  // |kMinFramesPerSecond|) video-sized updates to this window.
  base::TimeTicks update_times_[kMinFramesPerSecond];

  // Time at which the current sequence of updates that looks like video
  // started. Empty if video isn't currently playing.
  base::TimeTicks video_start_time_;

  // Index into |update_times_| of the oldest update.
  size_t buffer_start_;

  // Number of updates stored in |update_times_|.
  size_t buffer_size_;

  DISALLOW_COPY_AND_ASSIGN(ClientInfo);
};

VideoDetector::VideoDetector(FrameSinkManagerImpl* frame_sink_manager)
    : frame_sink_manager_(frame_sink_manager) {
  frame_sink_manager_->AddObserver(this);
}

VideoDetector::~VideoDetector() {
  frame_sink_manager_->RemoveObserver(this);
}

void VideoDetector::OnCompositorFrameReceived(const FrameSinkId& frame_sink_id,
                                              const CompositorFrame& frame) {
  // LOG(ERROR) << "RECEIVED";
  const gfx::Rect& damage_rect = frame.render_pass_list.back()->damage_rect;
  float device_scale_factor = frame.metadata.device_scale_factor;
  gfx::Rect damage_rect_in_dip =
      gfx::ConvertRectToDIP(device_scale_factor, damage_rect);

  std::unique_ptr<ClientInfo>& info = client_infos_[frame_sink_id];
  if (!info.get())
    info.reset(new ClientInfo);

  base::TimeTicks now = base::TimeTicks::Now();
  if (info->RecordUpdateAndCheckForVideo(damage_rect_in_dip, now))
    // LOG(ERROR) << "PLAYING";
    OnVideoActivityStarted(/* frame_sink_id */);
}

void VideoDetector::OnVideoActivityStarted(
    // const FrameSinkId& frame_sink_id
    ) {
  // TODO(samans): check visibility? Invisible clients don't seem to send frames
  // though.
  video_inactive_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kVideoTimeoutMs), this,
      &VideoDetector::OnVideoActivityEnded);
  if (!video_is_playing_) {
    video_is_playing_ = true;
    clients_.ForAllPtrs([](mojom::VideoDetectorClient* client) {
      client->OnVideoActivityStarted();
    });
    LOG(ERROR) << "[Viz] Video activity started";
  }
}

void VideoDetector::OnVideoActivityEnded() {
  DCHECK(video_is_playing_);
  video_is_playing_ = false;
  clients_.ForAllPtrs([](mojom::VideoDetectorClient* client) {
    client->OnVideoActivityEnded();
  });
  LOG(ERROR) << "[Viz] Video activity ended";
}

void VideoDetector::AddClient(mojom::VideoDetectorClientPtr client) {
  if (video_is_playing_)
    client->OnVideoActivityStarted();
  clients_.AddPtr(std::move(client));
}

}  // namespace viz
