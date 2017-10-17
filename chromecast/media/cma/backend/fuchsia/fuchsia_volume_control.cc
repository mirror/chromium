// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/system_volume_control.h"

namespace chromecast {
namespace media {

// static
std::unique_ptr<SystemVolumeControl> SystemVolumeControl::Create(
    Delegate* delegate) {
  // SystemVolumeControl is not implemented on Fuchsia. Volume will be applied
  // by the StreamMixer.
  return nullptr;
}

}  // namespace media
}  // namespace chromecast
