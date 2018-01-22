// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_ACCELERATORS_MEDIA_KEYS_LISTENER_H_
#define UI_BASE_ACCELERATORS_MEDIA_KEYS_LISTENER_H_

#include <memory>

#include "base/callback.h"
#include "ui/base/ui_base_export.h"

namespace ui {
class Accelerator;
}

namespace ui {

// Create MediaKeyListener to receive accelerators on media keys.
class UI_BASE_EXPORT MediaKeysListener {
 public:
  enum class Scope {
    kGlobal,  // Listener works only on global accelerators.
    kChrome,  // Listener works only on chrome accelerators.
  };

  // Use void return type to be compatible with WeakCalls.
  // |was_handled| always non nullptr.
  using MediaKeysAcceleratorReceiver =
      base::RepeatingCallback<void(const Accelerator& accelerator,
                                   bool* was_handled)>;

  virtual ~MediaKeysListener() = default;

  virtual void StartWatchingMediaKeys() = 0;
  virtual void StopWatchingMediaKeys() = 0;
  virtual bool IsWatchingMediaKeys() const = 0;
};

// Can return nullptr if media keys listening is not implemented.
UI_BASE_EXPORT std::unique_ptr<MediaKeysListener> CreateMediaKeysListener(
    MediaKeysListener::MediaKeysAcceleratorReceiver
        media_keys_accelerator_receiver,
    MediaKeysListener::Scope scope);

}  // namespace ui

#endif  // UI_BASE_ACCELERATORS_MEDIA_KEYS_LISTENER_H_
