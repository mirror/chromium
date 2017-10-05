// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef dasdfadfgads
#define dasdfadfgads

#include <unordered_map>

#include "base/timer/timer.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/service/frame_sinks/frame_sink_observer.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/viz/public/interfaces/compositing/video_detector_client.mojom.h"

namespace viz {

class FrameSinkManagerImpl;

class VideoDetector : public FrameSinkObserver {
 public:
  VideoDetector(FrameSinkManagerImpl* frame_sink_manager);
  ~VideoDetector() override;
  void AddClient(mojom::VideoDetectorClientPtr client);

 private:
  class ClientInfo;

  void OnVideoActivityStarted(/*const FrameSinkId& frame_sink_id*/);
  void OnVideoActivityEnded(/*const FrameSinkId& frame_sink_id*/);

  // FrameSinkObserver implementation.
  void OnCompositorFrameReceived(const FrameSinkId& frame_sink_id,
                                 const CompositorFrame& frame) override;

  // True if video has been observed in the last |kVideoTimeoutMs|.
  bool video_is_playing_ = false;

  // Calls HandleVideoInactive().
  base::OneShotTimer video_inactive_timer_;

  mojo::InterfacePtrSet<mojom::VideoDetectorClient> clients_;

  FrameSinkManagerImpl* const frame_sink_manager_;
  std::unordered_map<FrameSinkId, std::unique_ptr<ClientInfo>, FrameSinkIdHash>
      client_infos_;
};

}  // namespace viz

#endif  // dasdfadfgads
