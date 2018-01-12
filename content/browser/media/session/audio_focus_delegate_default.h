// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_SESSION_AUDIO_FOCUS_DELEGATE_DEFAULT_H_
#define CONTENT_BROWSER_MEDIA_SESSION_AUDIO_FOCUS_DELEGATE_DEFAULT_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "content/browser/media/session/audio_focus_delegate.h"
#include "content/browser/media/session/audio_focus_manager.h"

namespace content {

// AudioFocusDelegateDefault is the default implementation of
// AudioFocusDelegate which only handles audio focus between WebContents.
class AudioFocusDelegateDefault : public AudioFocusDelegate {
 public:
  explicit AudioFocusDelegateDefault(MediaSessionImpl* media_session);

  static std::unique_ptr<AudioFocusDelegate> Create(
      MediaSessionImpl* media_session);

  ~AudioFocusDelegateDefault() override;

  // AudioFocusDelegate implementation.
  bool RequestAudioFocus(
      AudioFocusManager::AudioFocusType audio_focus_type) override;
  void AbandonAudioFocus() override;

 private:
  // Weak pointer because |this| is owned by |media_session_|.
  MediaSessionImpl* media_session_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_SESSION_AUDIO_FOCUS_DELEGATE_DEFAULT_H_
