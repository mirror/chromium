// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_RESOURCE_SCHEDULER_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_RESOURCE_SCHEDULER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"

namespace blink {
class WebMediaPlayer;
class WebMediaPlayerClient;
}  // namespace blink

namespace content {

class MediaResourceScheduler {
 public:
  using CreateMediaPlayerCB =
      base::OnceCallback<std::unique_ptr<blink::WebMediaPlayer>(
          blink::WebMediaPlayerClient*)>;

  // Return the singleton scheduler for this process.
  // TODO(liberato): Move this to Page?
  static MediaResourceScheduler& Get();

  // Create a resource-aware player.
  blink::WebMediaPlayer* CreateMediaPlayer(CreateMediaPlayerCB create_cb,
                                           blink::WebMediaPlayerClient* client);

 protected:
  MediaResourceScheduler();
  ~MediaResourceScheduler();

  DISALLOW_COPY_AND_ASSIGN(MediaResourceScheduler);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_RESOURCE_SCHEDULER_H_
