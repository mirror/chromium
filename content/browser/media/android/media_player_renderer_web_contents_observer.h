// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_WEB_CONTENTS_OBSERVER_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_WEB_CONTENTS_OBSERVER_H_

#include <set>

#include "content/public/browser/web_contents_observer.h"

namespace content {

class MediaPlayerRenderer;

class MediaPlayerRendererWebContentsObserver : public WebContentsObserver {
 public:
  explicit MediaPlayerRendererWebContentsObserver(WebContents* web_contents);
  ~MediaPlayerRendererWebContentsObserver() override;

  void AddMediaPlayerRenderer(MediaPlayerRenderer* player);
  void RemoveMediaPlayerRenderer(MediaPlayerRenderer* player);

  // WebContentsObserver implementation.
  void DidUpdateAudioMutingState(bool muted) override;

 private:
  std::set<MediaPlayerRenderer*> players_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerRendererWebContentsObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_PLAYER_RENDERER_WEB_CONTENTS_OBSERVER_H_
