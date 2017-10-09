// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_MAC_COREAUDIO_DISPATCH_OVERRIDE_H_
#define MEDIA_AUDIO_MAC_COREAUDIO_DISPATCH_OVERRIDE_H_

namespace media {
// Initializes the CoreAudio hotfix, if supported. Calls to this function must
// be serialized. Will do nothing if called when already initialized.
bool InitializeCoreAudioDispatchOverride();
}  // namespace media

#endif  // MEDIA_AUDIO_MAC_COREAUDIO_DISPATCH_OVERRIDE_H_
