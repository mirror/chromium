// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_WRAPPER_H_
#define REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_WRAPPER_H_

namespace remoting {

class AsyncAudioSupplier;

class AudioPlayerIosWrapper {
 public:
  static bool AudioPlayerIosResetAudioPlayer(AsyncAudioSupplier* audioSupplier,
                                             void* instance);
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_WRAPPER_H_
