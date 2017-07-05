// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/android_overlay_web_contents_observer.h"

#include "base/bind.h"

namespace content {

AndroidOverlayWebContentsObserver::AndroidOverlayWebContentsObserver(
    WebContents* web_contents,
    Client* client,
    bool is_initially_visible)
    : WebContentsObserver(web_contents),
      client_(client),
      web_contents_is_visible_(is_initially_visible) {
  DCHECK(web_contents);
}

AndroidOverlayWebContentsObserver::~AndroidOverlayWebContentsObserver() {
  DCHECK(live_overlays_map_.empty());
}

void AndroidOverlayWebContentsObserver::AddOverlay(
    RenderFrameHost* render_frame_host,
    DialogOverlayImpl* overlay) {
  DCHECK(render_frame_host);
  DCHECK(overlay);

  if (!live_overlays_map_.count(render_frame_host)) {
    live_overlays_map_[render_frame_host] =
        base::flat_set<DialogOverlayImpl*>();
  }

  overlay->SetOverlayStoppingCB(base::BindOnce(
      &AndroidOverlayWebContentsObserver::RemoveOverlay, base::Unretained(this),
      base::Unretained(render_frame_host), base::Unretained(overlay)));

  live_overlays_map_[render_frame_host].insert(overlay);
  client_->OverlayVisibilityChanged();
}

void AndroidOverlayWebContentsObserver::RemoveOverlay(
    RenderFrameHost* render_frame_host,
    DialogOverlayImpl* overlay) {
  // NOTE: |render_frame_host| and |overlay| may be in the process of being
  // destroyed, and should not be accessed.

  auto it = live_overlays_map_.find(render_frame_host);

  DCHECK(it != live_overlays_map_.end());

  it->second.erase(overlay);

  if (it->second.empty())
    live_overlays_map_.erase(it);

  SignalChangeOrRemoval();
}

void AndroidOverlayWebContentsObserver::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  StopOverlaysInFrame(render_frame_host);
}

void AndroidOverlayWebContentsObserver::RenderFrameHostChanged(
    RenderFrameHost* old_host,
    RenderFrameHost* new_host) {
  StopOverlaysInFrame(old_host);
}

void AndroidOverlayWebContentsObserver::StopOverlaysInFrame(
    RenderFrameHost* render_frame_host) {
  auto it = live_overlays_map_.find(render_frame_host);

  if (it == live_overlays_map_.end())
    return;

  for (DialogOverlayImpl* overlay : it->second) {
    overlay->SetOverlayStoppingCB(base::OnceClosure());
    overlay->RequestStop();
  }

  live_overlays_map_.erase(it);

  SignalChangeOrRemoval();
}

void AndroidOverlayWebContentsObserver::SignalChangeOrRemoval() {
  if (live_overlays_map_.empty())
    client_->RemoveObserverForWebContents(web_contents());
  else
    client_->OverlayVisibilityChanged();
}

void AndroidOverlayWebContentsObserver::WasShown() {
  web_contents_is_visible_ = true;
  client_->OverlayVisibilityChanged();
}

void AndroidOverlayWebContentsObserver::WasHidden() {
  web_contents_is_visible_ = false;
  client_->OverlayVisibilityChanged();
}

bool AndroidOverlayWebContentsObserver::ShouldUseOverlayMode() {
  return web_contents_is_visible_ && !live_overlays_map_.empty();
}

void AndroidOverlayWebContentsObserver::WebContentsDestroyed() {
  // We should have received FrameDestruction notifications for each frame
  // and we should no longer have any overlays present.
  DCHECK(live_overlays_map_.empty());
  client_->RemoveObserverForWebContents(web_contents());
}

}  // namespace content
