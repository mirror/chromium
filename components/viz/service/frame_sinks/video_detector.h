// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_FRAME_SINKS_VIDEO_DETECTOR_H_
#define COMPONENTS_VIZ_SERVICE_FRAME_SINKS_VIDEO_DETECTOR_H_

#include <unordered_map>

#include "base/timer/timer.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/service/surfaces/surface_observer.h"
#include "components/viz/service/viz_service_export.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"

namespace viz {

class FrameSinkManagerImpl;
class VideoDetectorTest;

// Watches for updates to clients and tries to detect when a video is playing.
// We err on the side of false positives and can be fooled by things like
// continuous scrolling of a page.
class VIZ_SERVICE_EXPORT VideoDetector : public SurfaceObserver {
 public:
  explicit VideoDetector(FrameSinkManagerImpl* frame_sink_manager);
  ~VideoDetector();

  // Adds an observer. The observer can be removed by closing the mojo
  // connection.
  void AddObserver(mojom::VideoDetectorObserverPtr observer);

  // We need to delete the corresponding element in |client_infos_| when
  // |frame_sink_id| is invalidated in order to avoid leaking memory.
  void OnFrameSinkIdInvalidated(const FrameSinkId& frame_sink_id);

 private:
  friend class VideoDetectorTest;

  class ClientInfo;

  // Minimum dimensions in pixels that damages must have to be considered a
  // potential video frame.
  static constexpr int kMinUpdateWidth = 333;
  static constexpr int kMinUpdateHeight = 250;

  // Number of video-sized updates that we must see within a second in a client
  // before we assume that a video is playing.
  static constexpr int kMinFramesPerSecond = 15;

  // Timeout after which video is no longer considered to be playing.
  static constexpr base::TimeDelta kVideoTimeout =
      base::TimeDelta::FromMilliseconds(1000);

  // Duration video must be playing in a client before it is reported to
  // observers.
  static constexpr base::TimeDelta kMinVideoDuration =
      base::TimeDelta::FromMilliseconds(3000);

  // If no video activity is detected for |kVideoTimeout|, this
  // method will be called by |video_inactive_timer_|;
  void OnVideoActivityEnded();

  // SurfaceObserver implementation.
  void OnFirstSurfaceActivation(const SurfaceInfo& surface_info) override {}
  void OnSurfaceActivated(const SurfaceId& surface_id) override {}
  void OnSurfaceDestroyed(const SurfaceId& surface_id) override {}
  bool OnSurfaceDamaged(const SurfaceId& surface_id,
                        const BeginFrameAck& ack) override;
  void OnSurfaceDiscarded(const SurfaceId& surface_id) override {}
  void OnSurfaceDamageExpected(const SurfaceId& surface_id,
                               const BeginFrameArgs& args) override {}
  void OnSurfaceSubtreeDamaged(const SurfaceId& surface_id) override {}
  void OnSurfaceWillBeDrawn(const SurfaceId& surface_id) override;

  // True if video has been observed in the last |kVideoTimeout|.
  bool video_is_playing_ = false;

  // Provides the current time. Can be replaced in tests.
  std::unique_ptr<base::TickClock> tick_clock_;

  // Calls OnVideoActivityEnded() after |kVideoTimeout|. Uses |tick_clock_| to
  // measure time. Can be replaced in tests.
  std::unique_ptr<base::OneShotTimer> video_inactive_timer_;

  mojo::InterfacePtrSet<mojom::VideoDetectorObserver> observers_;

  FrameSinkManagerImpl* const frame_sink_manager_;

  base::flat_map<FrameSinkId, uint64_t> last_drawn_frame_index_;

  std::unordered_map<FrameSinkId, std::unique_ptr<ClientInfo>, FrameSinkIdHash>
      client_infos_;

  DISALLOW_COPY_AND_ASSIGN(VideoDetector);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_FRAME_SINKS_VIDEO_DETECTOR_H_
