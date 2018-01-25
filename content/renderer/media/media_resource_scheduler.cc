// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_resource_scheduler.h"

#include "base/memory/ptr_util.h"
#include "content/renderer/media/passthrough_media_player.h"
#include "content/renderer/media/passthrough_media_player_client.h"

namespace content {

class ResourceAwareMediaPlayer : public PassthroughMediaPlayer,
                                 public PassthroughMediaPlayerClient {
 public:
  ResourceAwareMediaPlayer(WebMediaPlayerClient* real_client)
      : PassthroughMediaPlayerClient(real_client) {}

  ~ResourceAwareMediaPlayer() override {
    // Clear the player before we destruct, so that any calls it makes to
    // the client will be handled properly.
    SetMediaPlayer(nullptr);
  }
};

// static
MediaResourceScheduler& MediaResourceScheduler::Get() {
  static MediaResourceScheduler* scheduler_ = new MediaResourceScheduler();

  return *scheduler_;
}

MediaResourceScheduler::MediaResourceScheduler() {}

MediaResourceScheduler::~MediaResourceScheduler() {}

blink::WebMediaPlayer* MediaResourceScheduler::CreateMediaPlayer(
    CreateMediaPlayerCB create_cb,
    blink::WebMediaPlayerClient* client) {
  ResourceAwareMediaPlayer* passthrough_player =
      new ResourceAwareMediaPlayer(client);

  passthrough_player->SetMediaPlayer(std::move(create_cb).Run(
      static_cast<blink::WebMediaPlayerClient*>(passthrough_player)));

  // Our caller is responsible for freeing |passthrough_player|.
  // TODO(liberato): RFI and friends should return a unique_ptr, rather than a
  // WebMediaPlayer* .
  return passthrough_player;
}

}  // namespace content
