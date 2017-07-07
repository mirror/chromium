// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/external_begin_frame_controller.h"

namespace ui {

ExternalBeginFrameController::ExternalBeginFrameController()
    : begin_frame_source_(this) {}

ExternalBeginFrameController::~ExternalBeginFrameController() {}

void ExternalBeginFrameController::SetClient(
    ExternalBeginFrameControllerClient* client) {
  client_ = client;
  client_->OnNeedsBeginFrames(needs_begin_frames_);
}

void ExternalBeginFrameController::IssueExternalBeginFrame(
    const cc::BeginFrameArgs& args) {
  begin_frame_source_.OnBeginFrame(args);
}

void ExternalBeginFrameController::OnNeedsBeginFrames(bool needs_begin_frames) {
  if (client_)
    client_->OnNeedsBeginFrames(needs_begin_frames);
  needs_begin_frames_ = needs_begin_frames;
}

void ExternalBeginFrameController::OnDisplayDidFinishFrame(
    const cc::BeginFrameAck& ack) {
  if (client_)
    client_->OnDisplayDidFinishFrame(ack);
}

}  // namespace ui
