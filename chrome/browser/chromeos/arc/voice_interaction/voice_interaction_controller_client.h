// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_CLIENT_H_

#include <memory>

#include "ash/public/cpp/voice_interaction_state.h"
#include "ash/public/interfaces/voice_interaction_controller.mojom.h"

namespace arc {

class VoiceInteractionControllerClient {
 public:
  explicit VoiceInteractionControllerClient();
  ~VoiceInteractionControllerClient();

  void NotifyVoiceInteractionStatusChanged(ash::VoiceInteractionState state);

  void NotifyVoiceInteractionSettingsEnabled(bool enabled);

  void NotifyVoiceInteractionContextEnabled(bool enabled);

  void NotifyVoiceInteractionSetupCompleted(bool completed);

  void SetControllerForTesting(
      ash::mojom::VoiceInteractionControllerPtr controller);

  void FlushMojoForTesting();

 private:
  void ConnectToVoiceInteractionController();

  ash::mojom::VoiceInteractionControllerPtr voice_interaction_controller_;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionControllerClient);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_CLIENT_H_
