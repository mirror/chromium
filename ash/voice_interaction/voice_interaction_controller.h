// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_H_
#define ASH_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/public/cpp/voice_interaction_state.h"
#include "ash/public/interfaces/voice_interaction_controller.mojom.h"
#include "ash/voice_interaction/voice_interaction_observer.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

class ASH_EXPORT VoiceInteractionController
    : public mojom::VoiceInteractionController {
 public:
  VoiceInteractionController();
  ~VoiceInteractionController() override;

  void BindRequest(mojom::VoiceInteractionControllerRequest request);

  void AddVoiceInteractionObserver(VoiceInteractionObserver* observer);
  void RemoveVoiceInteractionObserver(VoiceInteractionObserver* observer);

  // ash::mojom::VoiceInteractionController:
  void NotifyVoiceInteractionStatusChanged(
      VoiceInteractionState state) override;
  void NotifyVoiceInteractionSettingsEnabled(bool enabled) override;
  void NotifyVoiceInteractionContextEnabled(bool enabled) override;
  void NotifyVoiceInteractionSetupCompleted(bool completed) override;

  VoiceInteractionState voice_interaction_state() const {
    return voice_interaction_state_;
  }

  bool voice_interaction_settings_enabled() const {
    return voice_interaction_settings_enabled_;
  }

  bool voice_interaction_setup_completed() const {
    return voice_interaction_setup_completed_;
  }

 private:
  // Voice interaction state. The initial value should be set to STOPPED to make
  // sure the app list button burst animation could be correctly shown.
  VoiceInteractionState voice_interaction_state_ =
      VoiceInteractionState::STOPPED;

  // Whether voice interaction is enabled in system settings.
  bool voice_interaction_settings_enabled_ = false;

  // Whether voice intearction setup flow has completed.
  bool voice_interaction_setup_completed_ = false;

  base::ObserverList<VoiceInteractionObserver> voice_interaction_observers_;

  mojo::Binding<mojom::VoiceInteractionController> binding_;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionController);
};

}  // namespace ash

#endif  // ASH_VOICE_INTERACTION_VOICE_INTERACTION_CONTROLLER_H_
