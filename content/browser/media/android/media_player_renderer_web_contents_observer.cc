// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/android/media_player_renderer_web_contents_observer.h"

#include "content/browser/media/android/media_player_renderer.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    content::MediaPlayerRendererWebContentsObserver);

namespace content {

MediaPlayerRendererWebContentsObserver::MediaPlayerRendererWebContentsObserver(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

MediaPlayerRendererWebContentsObserver::
    ~MediaPlayerRendererWebContentsObserver() = default;

void MediaPlayerRendererWebContentsObserver::AddMediaPlayerRenderer(
    MediaPlayerRenderer* player) {
  DCHECK(player);
  auto insert_result = players_.insert(player);
  DCHECK(insert_result.second);
}

void MediaPlayerRendererWebContentsObserver::RemoveMediaPlayerRenderer(
    MediaPlayerRenderer* player) {
  DCHECK(player);
  auto erase_result = players_.erase(player);
  DCHECK_EQ(1u, erase_result);
}

void MediaPlayerRendererWebContentsObserver::DidUpdateAudioMutingState(
    bool muted) {
  for (auto it = players_.begin(); it != players_.end(); ++it) {
    MediaPlayerRenderer* player = *it;
    player->OnUpdateAudioMutingState(muted);
  }
}

void MediaPlayerRendererWebContentsObserver::WebContentsDestroyed() {
  for (auto it = players_.begin(); it != players_.end();) {
    MediaPlayerRenderer* player = *it;
    player->OnWebContentsDestroyed();
    it = players_.erase(it);
  }
}

}  // namespace content
