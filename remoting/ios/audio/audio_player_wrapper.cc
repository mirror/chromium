// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/ios/audio_player_wrapper.h"

#include <algorithm>

#include "base/logging.h"
#include "base/stl_util.h"
#include "remoting/client/audio_consumer.h"
#include "remoting/client/audio_supplier.h"
#include "remoting/client/ios/audio_player_buffer.h"

namespace remoting {

AudioPlayerWrapper::AudioPlayerWrapper() {
  // TODO(nicholss): Create the buffer and player objects.
}

AudioPlayerWrapper::~AudioPlayerWrapper() {}

void AudioPlayerWrapper::AddAudioPacket(std::unique_ptr<AudioPacket> packet) {
  audio_consumer_->AddAudioPacket(std::move(packet));
  if (audio_supplier_ != nullptr) {
    audio_supplier_->GetBufferedChannels();
  }
}

}  // namespace remoting
