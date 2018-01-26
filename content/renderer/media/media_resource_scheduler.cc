// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_resource_scheduler.h"

#include "base/memory/ptr_util.h"
#include "content/renderer/media/passthrough_media_player.h"
#include "content/renderer/media/passthrough_media_player_client.h"

namespace content {

class ResourceAwareMediaPlayer : public PassthroughMediaPlayerDelegate,
                                 public PassthroughMediaPlayerClient {
 public:
  ResourceAwareMediaPlayer(WebMediaPlayerClient* real_client)
      : PassthroughMediaPlayerClient(real_client) {}

  ~ResourceAwareMediaPlayer() override{};

  blink::WebMediaPlayerClient* GetClient() override {
    return static_cast<blink::WebMediaPlayerClient*>(this);
  }

  void OnPlay() override { controller()->PlayImpl(); }
};

// static
MediaResourceScheduler& MediaResourceScheduler::Get() {
  static MediaResourceScheduler* scheduler_ = new MediaResourceScheduler();

  return *scheduler_;
}

MediaResourceScheduler::MediaResourceScheduler() {}

MediaResourceScheduler::~MediaResourceScheduler() {}

std::unique_ptr<PassthroughMediaPlayerDelegate>
MediaResourceScheduler::CreateDelegate(blink::WebMediaPlayerClient* client) {
  return base::MakeUnique<ResourceAwareMediaPlayer>(client);
}

}  // namespace content
