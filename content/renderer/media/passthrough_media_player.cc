// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/passthrough_media_player.h"

#include "third_party/WebKit/public/platform/WebSize.h"

using blink::WebAudioSourceProvider;
using blink::WebCanvas;
using blink::WebContentDecryptionModule;
using blink::WebContentDecryptionModuleResult;
using blink::WebMediaPlayer;
using blink::WebMediaPlayerSource;
using blink::WebSecurityOrigin;
using blink::WebSetSinkIdCallbacks;
using blink::WebSize;
using blink::WebString;
using blink::WebTimeRanges;
using blink::WebRect;
using blink::WebURL;

namespace content {

PassthroughMediaPlayerController::PassthroughMediaPlayerController(
    std::unique_ptr<PassthroughMediaPlayerDelegate> delegate)
    : delegate_(std::move(delegate)) {
  delegate_->SetController(this);
}

PassthroughMediaPlayerController::~PassthroughMediaPlayerController() {
  delegate_->SetController(nullptr);
}

PassthroughMediaPlayerDelegate::~PassthroughMediaPlayerDelegate() {}

void PassthroughMediaPlayerDelegate::SetController(
    PassthroughMediaPlayerController* controller) {
  controller_ = controller;
}

}  // namespace content
