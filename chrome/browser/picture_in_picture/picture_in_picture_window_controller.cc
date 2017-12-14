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

#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/media/media_web_contents_observer.h"
#include "content/common/media/media_player_delegate_messages.h"

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
    window_ = OverlayWindow::Create(this);
    window_->Init(size);
  }
}

void PictureInPictureWindowController::Show(const gfx::Size& size) {
  if (window_ && window_->IsActive())
    return;

  if (!window_) {
    window_ = OverlayWindow::Create(this);
    window_->Init(size);
  }
  window_->Show();
}

void PictureInPictureWindowController::Close() {
  // TODO: we should show the video back in the tab.
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
  DCHECK(window_);
  embedder_.reset(new OverlaySurfaceEmbedder(window_.get()));

  // Reuse the LocalSurfaceId
  viz::LocalSurfaceId local_surface_id = viz::LocalSurfaceId(parent_id, nonce);
  viz::SurfaceId new_surface_id =
      viz::SurfaceId(frame_sink_id, local_surface_id);
  surface_id_ = new_surface_id;

  embedder_->SetPrimarySurfaceId(new_surface_id);
}

void PictureInPictureWindowController::TogglePlayPause() {
  content::MediaWebContentsObserver* observer = static_cast<content::WebContentsImpl* const>(initiator_)->media_web_contents_observer();
  base::Optional<content::WebContentsObserver::MediaPlayerId> player_id = observer->GetPictureInPictureVideoMediaPlayerId();
  if (!player_id.has_value()) {
    LOG(ERROR) << "this really should be a dcheck when the code will be less wip";
    return;
  }

  if (observer->IsActivePlayer(*player_id))
    player_id->first->Send(new MediaPlayerDelegateMsg_Pause(player_id->first->GetRoutingID(), player_id->second));
  else
    player_id->first->Send(new MediaPlayerDelegateMsg_Play(player_id->first->GetRoutingID(), player_id->second));
}