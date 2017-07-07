// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/media/MediaPlayer.h"

namespace blink {

MediaError* MediaPlayer::error() const {
  return nullptr;
}

const String& MediaPlayer::src() const {
  return src_;
}

void MediaPlayer::setSrc(const String& src) {
  src_ = src;
}

const String& MediaPlayer::crossOrigin() const {
  return cross_origin_;
}

void MediaPlayer::setCrossOrigin(const String& origin) {
  cross_origin_ = origin;
}

MediaPlayer::NetworkState MediaPlayer::getNetworkState() const {
  return kNetworkEmpty;
}

String MediaPlayer::preload() const {
  return String();
}

void MediaPlayer::setPreload(const String&) {}

TimeRanges* MediaPlayer::buffered() const {
  return nullptr;
}

void MediaPlayer::load() {}

String MediaPlayer::canPlayType(const String& mime_type) const {
  return String();
}

MediaPlayer::ReadyState MediaPlayer::getReadyState() const {
  return kHaveNothing;
}

bool MediaPlayer::seeking() const {
  return false;
}

double MediaPlayer::currentTime() const {
  return 0;
}

void MediaPlayer::setCurrentTime(double) {}

double MediaPlayer::duration() const {
  return 0;
}

bool MediaPlayer::paused() const {
  return false;
}

double MediaPlayer::defaultPlaybackRate() const {
  return 0;
}

void MediaPlayer::setDefaultPlaybackRate(double) {}

double MediaPlayer::playbackRate() const {
  return 0;
}

void MediaPlayer::setPlaybackRate(double) {}

TimeRanges* MediaPlayer::played() {
  return nullptr;
}

TimeRanges* MediaPlayer::seekable() const {
  return nullptr;
}

bool MediaPlayer::ended() const {
  return false;
}

bool MediaPlayer::autoplay() const {
  return false;
}

void MediaPlayer::setAutoplay(bool) {}

bool MediaPlayer::loop() const {
  return false;
}

void MediaPlayer::setLoop(bool) {}

ScriptPromise MediaPlayer::playForBindings(ScriptState* state) {
  return ScriptPromise::CastUndefined(state);
}

void MediaPlayer::pause() {}

double MediaPlayer::volume() const {
  return 0;
}

void MediaPlayer::setVolume(double, ExceptionState&) {}

bool MediaPlayer::muted() const {
  return false;
}

void MediaPlayer::setMuted(bool) {}

bool MediaPlayer::defaultMuted() const {
  return false;
}

void MediaPlayer::setDefaultMuted(bool) {}

AudioTrackList* MediaPlayer::audioTracks() {
  return nullptr;
}

VideoTrackList* MediaPlayer::videoTracks() {
  return nullptr;
}

TextTrackList* MediaPlayer::textTracks() {
  return nullptr;
}

TextTrack* MediaPlayer::addTextTrack(const String& kind,
                                     const String& label,
                                     const String& language,
                                     ExceptionState&) {
  return nullptr;
}

unsigned MediaPlayer::webkitAudioDecodedByteCount() const {
  return 0;
}

unsigned MediaPlayer::webkitVideoDecodedByteCount() const {
  return 0;
}

}  // namespace blink
