// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_VOICE_INTERACTION_VOICE_INTERACTION_OBSERVER_H_
#define ASH_VOICE_INTERACTION_VOICE_INTERACTION_OBSERVER_H_

#include "ash/public/interfaces/voice_interaction_controller.mojom.h"

namespace ash {

class VoiceInteractionObserver {
 public:
  // Called when voice interaction session state changes.
  virtual void OnVoiceInteractionStatusChanged(
      mojom::VoiceInteractionState state) {}

  // Called when voice interaction is enabled/disabled in settings.
  virtual void OnVoiceInteractionSettingsEnabled(bool enabled) {}

  // Called when voice interaction service is allowed/disallowed to access
  // the "context" (text and graphic content that is currently on screen).
  virtual void OnVoiceInteractionContextEnabled(bool enabled) {}

  // Called when voice interaction setup flow completed.
  virtual void OnVoiceInteractionSetupCompleted(bool completed) {}

 protected:
  virtual ~VoiceInteractionObserver() = default;
};

}  // namespace ash

#endif  // ASH_VOICE_INTERACTION_VOICE_INTERACTION_OBSERVER_H_
