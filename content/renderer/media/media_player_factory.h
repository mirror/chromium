// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerFactory.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"

namespace base {
class SingleThreadTaskRunner;
class TaskRunner;
}  // namespace base

namespace media {
class SwitchableAudioRendererSink;
class UrlIndex;
class WebMediaPlayerDelegate;
}  // namespace media

namespace content {
class AudioRendererMixerManager;

class MediaPlayerFactory : public blink::WebMediaPlayerFactory {
 public:
  MediaPlayerFactory(
      int routing_id,
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
      scoped_refptr<base::TaskRunner> worker_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner);
  ~MediaPlayerFactory() override;

  // blink::WebMediaPlayerFactory implementation.
  void SetSecurityOrigin(const blink::WebSecurityOrigin& origin) override;
  std::unique_ptr<blink::WebMediaPlayer> CreateMediaPlayer(
      const blink::WebMediaPlayerSource& source,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
      blink::WebContentDecryptionModule* initial_cdm,
      const blink::WebString& sink_id) override;

 private:
  scoped_refptr<media::SwitchableAudioRendererSink> NewMixableSink(
      const blink::WebString& sink_id);
  media::WebMediaPlayerDelegate* GetWebMediaPlayerDelegate();

  int routing_id_;
  blink::WebSecurityOrigin security_origin_;
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  scoped_refptr<base::TaskRunner> worker_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  linked_ptr<media::UrlIndex> url_index_;
  std::unique_ptr<AudioRendererMixerManager> audio_renderer_mixer_manager_;
  std::unique_ptr<media::WebMediaPlayerDelegate> media_player_delegate_;
};

}  // namespace content
