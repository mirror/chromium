// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CONTROLLER_H_
#define UI_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CONTROLLER_H_

#include "cc/output/begin_frame_args.h"
#include "cc/scheduler/begin_frame_source.h"
#include "cc/surfaces/display.h"
#include "ui/compositor/compositor_export.h"

namespace ui {

class COMPOSITOR_EXPORT ExternalBeginFrameControllerClient {
 public:
  virtual ~ExternalBeginFrameControllerClient() {}

  virtual void OnDisplayDidFinishFrame(const cc::BeginFrameAck& ack) = 0;
  virtual void OnNeedsBeginFrames(bool needs_begin_frames) = 0;
};

class COMPOSITOR_EXPORT ExternalBeginFrameController
    : NON_EXPORTED_BASE(public cc::ExternalBeginFrameSourceClient),
      NON_EXPORTED_BASE(public cc::DisplayObserver) {
 public:
  ExternalBeginFrameController();
  ~ExternalBeginFrameController() override;

  // Set the client that is notified when BeginFrames are needed and completed.
  void SetClient(ExternalBeginFrameControllerClient* client);

  // Issue a BeginFrame with the given |args|.
  void IssueExternalBeginFrame(const cc::BeginFrameArgs& args);

  cc::BeginFrameSource* begin_frame_source() { return &begin_frame_source_; }

 private:
  // cc::ExternalBeginFrameSourceClient implementation.
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

  // cc::DisplayObserver implementation.
  void OnDisplayDidFinishFrame(const cc::BeginFrameAck& ack) override;

  cc::ExternalBeginFrameSource begin_frame_source_;
  ExternalBeginFrameControllerClient* client_ = nullptr;
  bool needs_begin_frames_ = false;
};

}  // namespace ui

#endif  // UI_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CONTROLLER_H_
