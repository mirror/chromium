// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/audio_focus_delegate_default.h"

#include "base/command_line.h"
#include "media/base/media_switches.h"

namespace content {

using AudioFocusType = AudioFocusManager::AudioFocusType;

AudioFocusDelegateDefault::AudioFocusDelegateDefault(
    MediaSessionImpl* media_session)
    : media_session_(media_session) {}

AudioFocusDelegateDefault::~AudioFocusDelegateDefault() = default;

bool AudioFocusDelegateDefault::RequestAudioFocus(
    AudioFocusManager::AudioFocusType audio_focus_type) {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableAudioFocus)) {
    return true;
  }

  AudioFocusManager::GetInstance()->RequestAudioFocus(media_session_,
                                                      audio_focus_type);
  return true;
}

void AudioFocusDelegateDefault::AbandonAudioFocus() {
  AudioFocusManager::GetInstance()->AbandonAudioFocus(media_session_);
}

// static
std::unique_ptr<AudioFocusDelegate> AudioFocusDelegateDefault::Create(
    MediaSessionImpl* media_session) {
  return std::unique_ptr<AudioFocusDelegate>(
      new AudioFocusDelegateDefault(media_session));
}

}  // namespace content
