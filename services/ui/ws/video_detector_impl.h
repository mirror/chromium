// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef afdafeqdsada
#define afdafeqdsada

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/ui/public/interfaces/video_detector.mojom.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"

namespace viz {
class HostFrameSinkManager;
}

namespace ui {
namespace ws {

class VideoDetectorImpl : public mojom::VideoDetector {
 public:
  VideoDetectorImpl(viz::HostFrameSinkManager* host_frame_sink_manager);
  ~VideoDetectorImpl() override;

  void AddBinding(mojom::VideoDetectorRequest request);

  // mojom::VideoDetector implementation.
  void AddObserver(viz::mojom::VideoDetectorObserverPtr observer) override;

 private:
  viz::HostFrameSinkManager* host_frame_sink_manager_;
  mojo::BindingSet<mojom::VideoDetector> binding_set_;
};

}  // namespace ws
}  // namespace ui

#endif  // afdafeqdsada
