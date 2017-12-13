// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/picture_in_picture/picture_in_picture_window_controller.h"

#include "chrome/browser/ui/overlay/overlay_surface_embedder.h"
#include "chrome/browser/ui/overlay/overlay_window.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PictureInPictureWindowController);

// static
PictureInPictureWindowController*
PictureInPictureWindowController::GetOrCreateForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);

  // This is a no-op if the controller already exists.
  CreateForWebContents(web_contents);
  return FromWebContents(web_contents);
}

PictureInPictureWindowController::~PictureInPictureWindowController() {
  if (window_)
    window_->Close();
}

PictureInPictureWindowController::PictureInPictureWindowController(
    content::WebContents* initiator)
    : initiator_(initiator) {
  DCHECK(initiator_);
}

void PictureInPictureWindowController::Init(const gfx::Size& size) {
  if (!window_) {
    window_ = OverlayWindow::Create();
    window_->Init(size);
  }
}

void PictureInPictureWindowController::Show(const gfx::Size& size) {
  if (window_ && window_->IsActive())
    return;

  if (!window_) {
    window_ = OverlayWindow::Create();
    window_->Init(size);
  }
  window_->Show();
}

void PictureInPictureWindowController::Close() {
  if (window_->IsActive())
    window_->Close();
}

void PictureInPictureWindowController::SetFrameSinkId(
    viz::FrameSinkId frame_sink_id) {
  frame_sink_id_ = frame_sink_id;
}

void PictureInPictureWindowController::EmbedSurface(
    viz::FrameSinkId frame_sink_id,
    uint32_t parent_id,
    base::UnguessableToken nonce) {
  LOG(ERROR) << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
  LOG(ERROR) << "PictureInPictureWindowController::EmbedSurface";
  DCHECK(window_);
  embedder_.reset(new OverlaySurfaceEmbedder(window_.get()));

  // Reuse the LocalSurfaceId
  viz::LocalSurfaceId local_surface_id = viz::LocalSurfaceId(parent_id, nonce);
  viz::SurfaceId new_surface_id =
      viz::SurfaceId(frame_sink_id, local_surface_id);
  surface_id_ = new_surface_id;

  // LOG(ERROR) << frame_sink_id << " | " << parent_id << " | " << nonce;
  embedder_->SetPrimarySurfaceId(new_surface_id);
}
