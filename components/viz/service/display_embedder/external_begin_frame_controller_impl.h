// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_EXTERNAL_BEGIN_FRAME_CONTROLLER_IMPL_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_EXTERNAL_BEGIN_FRAME_CONTROLLER_IMPL_H_

#include <memory>

#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/service/display/display.h"
#include "services/viz/privileged/interfaces/compositing/external_begin_frame_controller.mojom.h"

namespace viz {

class VIZ_SERVICE_EXPORT ExternalBeginFrameControllerImpl
    : public mojom::ExternalBeginFrameController,
      public ExternalBeginFrameSourceClient,
      public DisplayObserver {
 public:
  ExternalBeginFrameControllerImpl();
  ~ExternalBeginFrameControllerImpl() override;

  // Issue a BeginFrame with the given |args|.
  void IssueExternalBeginFrame(const BeginFrameArgs& args) override;

  BeginFrameSource* begin_frame_source() { return &begin_frame_source_; }

  void SetDisplay(Display* display);

  void SetClient(mojom::ExternalBeginFrameControllerClientPtr client);

 private:
  // ExternalBeginFrameSourceClient implementation.
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

  // DisplayObserver implementation.
  void OnDisplayDidFinishFrame(const BeginFrameAck& ack) override;

  ExternalBeginFrameSource begin_frame_source_;
  bool needs_begin_frames_ = false;
  Display* display_ = nullptr;
  mojom::ExternalBeginFrameControllerClientPtr client_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_EXTERNAL_BEGIN_FRAME_CONTROLLER_IMPL_H_
