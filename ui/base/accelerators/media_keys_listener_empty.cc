// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/accelerators/media_keys_listener.h"

namespace ui {

std::unique_ptr<MediaKeysListener> CreateMediaKeysListener(
    MediaKeysListener::MediaKeysAcceleratorReceiver
        media_keys_accelerator_receiver,
    MediaKeysListener::Scope scope) {
  return nullptr;
}

}  // namespace ui
