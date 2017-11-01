// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/video_detector.h"

#include "base/time/time.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/rect.h"

namespace viz {

constexpr base::TimeDelta VideoDetector::kVideoTimeout;
constexpr base::TimeDelta VideoDetector::kMinVideoDuration;

// Stores information about updates to a client and determines whether it's
// likely that a video is playing in it.
class VideoDetector::ClientInfo {
 public:
  ClientInfo() {}

  // Handles an update within a client, returning true if it appears that
  // video is currently playing in the client.
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
    return in_video && elapsed >= kMinVideoDuration;
  }

 private:
  // Circular buffer containing update times of the last (up to
  // |kMinFramesPerSecond|) video-sized updates to this client.
  base::TimeTicks update_times_[kMinFramesPerSecond];

  // Time at which the current sequence of updates that looks like video
  // started. Empty if video isn't currently playing.
  base::TimeTicks video_start_time_;

  // Index into |update_times_| of the oldest update.
  size_t buffer_start_ = 0;

  // Number of updates stored in |update_times_|.
  size_t buffer_size_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ClientInfo);
};

VideoDetector::VideoDetector(FrameSinkManagerImpl* frame_sink_manager)
    : frame_sink_manager_(frame_sink_manager) {
  frame_sink_manager_->surface_manager()->AddObserver(this);
}

VideoDetector::~VideoDetector() = default;

void VideoDetector::OnVideoActivityEnded() {
  DCHECK(video_is_playing_);
  video_is_playing_ = false;
  observers_.ForAllPtrs([](mojom::VideoDetectorObserver* observer) {
    observer->OnVideoActivityEnded();
  });
}

void VideoDetector::AddObserver(mojom::VideoDetectorObserverPtr observer) {
  if (video_is_playing_)
    observer->OnVideoActivityStarted();
  observers_.AddPtr(std::move(observer));
}

bool VideoDetector::OnSurfaceDamaged(const SurfaceId& surface_id,
                                     const BeginFrameAck& ack) {
  return false;
}

// |surface_id| is scheduled to be drawn. See if it has a new frame since the
// last time it was drawn and record the damage.
void VideoDetector::OnSurfaceWillBeDrawn(const SurfaceId& surface_id) {
  // If there is no observer, don't waste cycles detecting video activity.
  if (observers_.empty())
    return;

  Surface* surface =
      frame_sink_manager_->surface_manager()->GetSurfaceForId(surface_id);
  DCHECK(surface->HasActiveFrame());
  const FrameSinkId& frame_sink_id = surface->surface_id().frame_sink_id();

  uint64_t frame_index = surface->GetActiveFrameIndex();

  // If |frame_index| is smaller than what we saw last time, it means that this
  // is a new client and the frame sink id is being reused.
  if (frame_index < last_drawn_frame_index_[frame_sink_id]) {
    last_drawn_frame_index_.erase(frame_sink_id);
    client_infos_.erase(frame_sink_id);
  }

  // If |frame_index| hasn't increased, then no new frame was submitted since
  // the last draw.
  if (frame_index == last_drawn_frame_index_[frame_sink_id])
    return;

  last_drawn_frame_index_[frame_sink_id] = frame_index;

  const CompositorFrame& frame = surface->GetActiveFrame();
  const gfx::Rect& damage_rect = frame.render_pass_list.back()->damage_rect;
  gfx::Rect damage_rect_in_dip =
      gfx::ConvertRectToDIP(frame.device_scale_factor(), damage_rect);

  std::unique_ptr<ClientInfo>& info = client_infos_[frame_sink_id];
  if (!info.get())
    info = std::make_unique<ClientInfo>();

  base::TimeTicks now =
      !now_for_testing_.is_null() ? now_for_testing_ : base::TimeTicks::Now();

  if (info->RecordUpdateAndCheckForVideo(damage_rect_in_dip, now)) {
    video_inactive_timer_.Start(FROM_HERE, kVideoTimeout, this,
                                &VideoDetector::OnVideoActivityEnded);
    if (!video_is_playing_) {
      video_is_playing_ = true;
      observers_.ForAllPtrs([](mojom::VideoDetectorObserver* observer) {
        observer->OnVideoActivityStarted();
      });
    }
  }
}

bool VideoDetector::TriggerTimeoutForTesting() {
  if (!video_inactive_timer_.IsRunning())
    return false;

  video_inactive_timer_.Stop();
  OnVideoActivityEnded();
  return true;
}

}  // namespace viz
