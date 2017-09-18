// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_player_factory.h"

#include "content/renderer/media/audio_renderer_mixer_manager.h"
#include "content/renderer/media/renderer_webmediaplayer_delegate.h"
#include "content/renderer/render_frame_impl.h"
#include "media/base/audio_renderer_mixer_input.h"
#include "media/base/renderer_factory_selector.h"
#include "media/blink/webmediaplayer_impl.h"
#include "media/renderers/default_renderer_factory.h"

namespace content {

namespace {
class FakeWebMediaPlayerDelegate : public media::WebMediaPlayerDelegate {
 public:
  FakeWebMediaPlayerDelegate() = default;
  ~FakeWebMediaPlayerDelegate() override = default;

  int AddObserver(Observer* observer) override {
    observer_ = observer;
    return delegate_id_;
  }
  void RemoveObserver(int delegate_id) override {
    DCHECK_EQ(delegate_id_, delegate_id);
    observer_ = nullptr;
  }
  void DidPlay(int delegate_id,
               bool has_video,
               bool has_audio,
               media::MediaContentType type) override {
    DCHECK_EQ(delegate_id_, delegate_id);
    DCHECK(!playing_);
    playing_ = true;
    has_video_ = has_video;
    is_gone_ = false;
  }
  void DidPlayerMutedStatusChange(int delegate_id, bool muted) override {
    DCHECK_EQ(delegate_id_, delegate_id);
  }
  void DidPause(int delegate_id) override {
    DCHECK_EQ(delegate_id_, delegate_id);
    DCHECK(playing_);
    DCHECK(!is_gone_);
    playing_ = false;
  }
  void DidPlayerSizeChange(int delegate_id, const gfx::Size& size) override {
    DCHECK_EQ(delegate_id_, delegate_id);
  }
  void PlayerGone(int delegate_id) override {
    DCHECK_EQ(delegate_id_, delegate_id);
    is_gone_ = true;
  }
  void SetIdle(int delegate_id, bool is_idle) override {
    DCHECK_EQ(delegate_id_, delegate_id);
    is_idle_ = is_idle;
  }
  bool IsIdle(int delegate_id) override {
    DCHECK_EQ(delegate_id_, delegate_id);
    return is_idle_;
  }
  void ClearStaleFlag(int delegate_id) override {
    DCHECK_EQ(delegate_id_, delegate_id);
  }
  bool IsStale(int delegate_id) override {
    DCHECK_EQ(delegate_id_, delegate_id);
    return false;
  }
  void SetIsEffectivelyFullscreen(int delegate_id,
                                  bool is_fullscreen) override {
    DCHECK_EQ(delegate_id_, delegate_id);
  }
  bool IsFrameHidden() override { return is_hidden_; }
  bool IsFrameClosed() override { return false; }

 private:
  int delegate_id_ = 1234;
  Observer* observer_ = nullptr;
  bool playing_ = false;
  bool has_video_ = false;
  bool is_hidden_ = false;
  bool is_gone_ = true;
  bool is_idle_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeWebMediaPlayerDelegate);
};
}  // namespace

MediaPlayerFactory::MediaPlayerFactory(
    int routing_id,
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    scoped_refptr<base::TaskRunner> worker_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner)
    : routing_id_(routing_id),
      media_task_runner_(std::move(media_task_runner)),
      worker_task_runner_(std::move(worker_task_runner)),
      compositor_task_runner_(std::move(compositor_task_runner)) {}

MediaPlayerFactory::~MediaPlayerFactory() = default;

void MediaPlayerFactory::Initialize(blink::WebFetchContext* fetch_context) {
  fetch_context_.reset(fetch_context);
}

std::unique_ptr<blink::WebMediaPlayer> MediaPlayerFactory::CreateMediaPlayer(
    const blink::WebMediaPlayerSource& source,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    blink::WebContentDecryptionModule* initial_cdm,
    const blink::WebString& sink_id) {
  media::WebMediaPlayerParams::DeferLoadCB defer_load_cb;
  media::WebMediaPlayerParams::Context3DCB context_3d_cb;
  media::WebMediaPlayerParams::AdjustAllocatedMemoryCB adjust_memory_cb =
      base::Bind(&v8::Isolate::AdjustAmountOfExternalAllocatedMemory,
                 base::Unretained(v8::Isolate::GetCurrent()));
  media::RequestRoutingTokenCallback request_routing_token_cb;
  bool embedded_media_experience_enabled = false;
  base::TimeDelta max_keyframe_distance_to_disable_background_video =
      base::TimeDelta::FromSeconds(10);
  base::TimeDelta max_keyframe_distance_to_disable_background_video_mse =
      base::TimeDelta::FromSeconds(10);
  bool enable_instant_source_buffer_gc = false;
  media::SurfaceManager* media_surface_manager = nullptr;
  base::WeakPtr<media::MediaObserver> media_observer;
  media::WebMediaPlayerParams::CreateCapabilitiesRecorderCB
      create_capabilities_recorder_cb;
  base::Callback<std::unique_ptr<blink::WebSurfaceLayerBridge>(
      blink::WebSurfaceLayerBridgeObserver*)>
      bridge_callback;

  auto media_log = base::MakeUnique<media::MediaLog>();
  auto factory_selector = base::MakeUnique<media::RendererFactorySelector>();
  factory_selector->AddFactory(
      media::RendererFactorySelector::FactoryType::DEFAULT,
      base::MakeUnique<media::DefaultRendererFactory>(
          media_log.get(), nullptr,
          media::DefaultRendererFactory::GetGpuFactoriesCB()));
  factory_selector->SetBaseFactoryType(
      media::RendererFactorySelector::FactoryType::DEFAULT);

  if (!url_index_)
    url_index_ = base::MakeUnique<media::UrlIndex>(fetch_context_.get());

  auto params = base::MakeUnique<media::WebMediaPlayerParams>(
      std::move(media_log), defer_load_cb, NewMixableSink(sink_id),
      media_task_runner_, worker_task_runner_, compositor_task_runner_,
      context_3d_cb, adjust_memory_cb, initial_cdm, media_surface_manager,
      request_routing_token_cb, media_observer,
      max_keyframe_distance_to_disable_background_video,
      max_keyframe_distance_to_disable_background_video_mse,
      enable_instant_source_buffer_gc, embedded_media_experience_enabled,
      watch_time_recorder_provider_.get(), create_capabilities_recorder_cb,
      bridge_callback);

  return base::MakeUnique<media::WebMediaPlayerImpl>(
      nullptr, client, encrypted_client, GetWebMediaPlayerDelegate(),
      std::move(factory_selector), url_index_.get(), std::move(params));
}

scoped_refptr<media::SwitchableAudioRendererSink>
MediaPlayerFactory::NewMixableSink(const blink::WebString& sink_id) {
  if (!audio_renderer_mixer_manager_) {
    audio_renderer_mixer_manager_ = AudioRendererMixerManager::Create();
  }
  return scoped_refptr<media::AudioRendererMixerInput>(
      audio_renderer_mixer_manager_->CreateInput(
          routing_id_, 0, sink_id.Utf8(), fetch_context_->GetSecurityOrigin(),
          media::AudioLatency::LATENCY_PLAYBACK));
}

media::WebMediaPlayerDelegate* MediaPlayerFactory::GetWebMediaPlayerDelegate() {
  if (!media_player_delegate_) {
    media_player_delegate_ = base::MakeUnique<FakeWebMediaPlayerDelegate>();
  }
  return media_player_delegate_.get();
}

}  // namespace content
