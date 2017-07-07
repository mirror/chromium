// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaPlayer_h
#define MediaPlayer_h

#include "bindings/core/v8/ExceptionState.h"
#include "core/CoreExport.h"
#include "platform/weborigin/KURL.h"

namespace blink {

class AudioTrackList;
class MediaError;
class ScriptPromise;
class ScriptState;
class TextTrack;
class TextTrackList;
class TimeRanges;
class VideoTrackList;

class CORE_EXPORT MediaPlayer {
 public:
  // error state
  MediaError* error() const;

  // network state
  const String& src() const;
  void setSrc(const String&);
  const KURL& currentSrc() const { return current_src_; }
  const String& crossOrigin() const;
  void setCrossOrigin(const String&);

  enum NetworkState {
    kNetworkEmpty,
    kNetworkIdle,
    kNetworkLoading,
    kNetworkNoSource
  };
  NetworkState getNetworkState() const;
  String preload() const;
  void setPreload(const String&);
  TimeRanges* buffered() const;
  void load();
  String canPlayType(const String& mime_type) const;

  // ready state
  enum ReadyState {
    kHaveNothing,
    kHaveMetadata,
    kHaveCurrentData,
    kHaveFutureData,
    kHaveEnoughData
  };
  ReadyState getReadyState() const;
  bool seeking() const;

  // playback state
  double currentTime() const;
  void setCurrentTime(double);
  double duration() const;
  bool paused() const;
  double defaultPlaybackRate() const;
  void setDefaultPlaybackRate(double);
  double playbackRate() const;
  void setPlaybackRate(double);
  TimeRanges* played();
  TimeRanges* seekable() const;
  bool ended() const;
  bool autoplay() const;
  void setAutoplay(bool);
  bool loop() const;
  void setLoop(bool);
  ScriptPromise playForBindings(ScriptState*);
  void pause();

  // controls
  double volume() const;
  void setVolume(double, ExceptionState& = ASSERT_NO_EXCEPTION);
  bool muted() const;
  void setMuted(bool);
  bool defaultMuted() const;
  void setDefaultMuted(bool);

  // tracks
  AudioTrackList* audioTracks();
  VideoTrackList* videoTracks();
  TextTrackList* textTracks();
  TextTrack* addTextTrack(const String& kind,
                          const String& label,
                          const String& language,
                          ExceptionState&);

  // statistics
  unsigned webkitAudioDecodedByteCount() const;
  unsigned webkitVideoDecodedByteCount() const;

 private:
  String src_;
  KURL current_src_;
  String cross_origin_;
};

}  // namespace blink

#endif  // MediaPlayer_h
