// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerMediaPlayer_h
#define WorkerMediaPlayer_h

#include "bindings/core/v8/ExceptionState.h"
#include "core/CoreExport.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/events/EventTarget.h"
#include "core/media/MediaPlayer.h"
#include "core/media/MediaPlayerClient.h"

namespace blink {

class AudioTrackList;
class MediaError;
class MediaPlayer;
class ScriptPromise;
class ScriptState;
class TextTrack;
class TextTrackList;
class TimeRanges;
class VideoTrackList;

class CORE_EXPORT WorkerMediaPlayer : public EventTargetWithInlineData,
                                      public MediaPlayerClient {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // EventTarget
  ExecutionContext* GetExecutionContext() const final;

  // error state
  MediaError* error() const { return media_player_->error(); }

  // network state
  const String& src() const { return media_player_->src(); }
  void setSrc(const String& src) { media_player_->setSrc(src); }
  const KURL& currentSrc() const { return media_player_->currentSrc(); }
  const String& crossOrigin() const { return media_player_->crossOrigin(); }
  void setCrossOrigin(const String& origin) {
    media_player_->setCrossOrigin(origin);
  }

  enum NetworkState {
    kNetworkEmpty = MediaPlayer::kNetworkEmpty,
    kNetworkIdle = MediaPlayer::kNetworkIdle,
    kNetworkLoading = MediaPlayer::kNetworkLoading,
    kNetworkNoSource = MediaPlayer::kNetworkNoSource
  };
  NetworkState getNetworkState() const {
    return static_cast<NetworkState>(media_player_->getNetworkState());
  }
  String preload() const { return media_player_->preload(); }
  void setPreload(const String& preload) { media_player_->setPreload(preload); }
  TimeRanges* buffered() const { return media_player_->buffered(); }
  void load() { media_player_->load(); }
  String canPlayType(const String& mime_type) const {
    return media_player_->canPlayType(mime_type);
  }

  // ready state
  enum ReadyState {
    kHaveNothing = MediaPlayer::kHaveNothing,
    kHaveMetadata = MediaPlayer::kHaveMetadata,
    kHaveCurrentData = MediaPlayer::kHaveCurrentData,
    kHaveFutureData = MediaPlayer::kHaveFutureData,
    kHaveEnoughData = MediaPlayer::kHaveEnoughData
  };
  ReadyState getReadyState() const {
    return static_cast<ReadyState>(media_player_->getReadyState());
  }
  bool seeking() const { return media_player_->seeking(); }

  // playback state
  double currentTime() const { return media_player_->currentTime(); }
  void setCurrentTime(double current_time) {
    media_player_->setCurrentTime(current_time);
  }
  double duration() const { return media_player_->duration(); }
  bool paused() const { return media_player_->paused(); }
  double defaultPlaybackRate() const {
    return media_player_->defaultPlaybackRate();
  }
  void setDefaultPlaybackRate(double rate) {
    media_player_->setDefaultPlaybackRate(rate);
  }
  double playbackRate() const { return media_player_->playbackRate(); }
  void setPlaybackRate(double rate) { media_player_->setPlaybackRate(rate); }
  TimeRanges* played() { return media_player_->played(); }
  TimeRanges* seekable() const { return media_player_->seekable(); }
  bool ended() const { return media_player_->ended(); }
  bool autoplay() const { return media_player_->autoplay(); }
  void setAutoplay(bool autoplay) { media_player_->setAutoplay(autoplay); }
  bool loop() const { return media_player_->loop(); }
  void setLoop(bool loop) { media_player_->setLoop(loop); }
  ScriptPromise playForBindings(ScriptState* script_state) {
    return media_player_->playForBindings(script_state);
  }
  void pause() { media_player_->pause(); }

  // controls
  double volume() const { return media_player_->volume(); }
  void setVolume(double volume,
                 ExceptionState& exception_state = ASSERT_NO_EXCEPTION) {
    media_player_->setVolume(volume, exception_state);
  }
  bool muted() const { return media_player_->muted(); }
  void setMuted(bool muted) { media_player_->setMuted(muted); }
  bool defaultMuted() const { return media_player_->defaultMuted(); }
  void setDefaultMuted(bool default_muted) {
    media_player_->setDefaultMuted(default_muted);
  }

  // tracks
  AudioTrackList* audioTracks() { return nullptr; }
  VideoTrackList* videoTracks() { return nullptr; }
  TextTrackList* textTracks() { return nullptr; }
  TextTrack* addTextTrack(const String& kind,
                          const String& label,
                          const String& language,
                          ExceptionState&) {
    return nullptr;
  }

  // statistics
  unsigned webkitAudioDecodedByteCount() const {
    return media_player_->webkitAudioDecodedByteCount();
  }
  unsigned webkitVideoDecodedByteCount() const {
    return media_player_->webkitVideoDecodedByteCount();
  }

  DECLARE_VIRTUAL_TRACE();

 protected:
  WorkerMediaPlayer(ExecutionContext*);

  // MediaPlayerClient implementation.
  std::unique_ptr<WebMediaPlayer> CreateWebMediaPlayer(
      const WebMediaPlayerSource&,
      WebMediaPlayerClient*) override;

 private:
  Member<MediaPlayer> media_player_;
};

}  // namespace blink

#endif  // WorkerMediaPlayer_h
