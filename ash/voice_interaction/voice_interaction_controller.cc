// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/voice_interaction/voice_interaction_controller.h"

namespace ash {

VoiceInteractionController::VoiceInteractionController() : binding_(this) {}

VoiceInteractionController::~VoiceInteractionController() {}

void VoiceInteractionController::BindRequest(
    mojom::VoiceInteractionControllerRequest request) {
  binding_.Bind(std::move(request));
}

void VoiceInteractionController::AddVoiceInteractionObserver(
    VoiceInteractionObserver* observer) {
  voice_interaction_observers_.AddObserver(observer);
}

void VoiceInteractionController::RemoveVoiceInteractionObserver(
    VoiceInteractionObserver* observer) {
  voice_interaction_observers_.RemoveObserver(observer);
}

void VoiceInteractionController::NotifyVoiceInteractionStatusChanged(
    VoiceInteractionState state) {
  voice_interaction_state_ = state;
  for (auto& observer : voice_interaction_observers_)
    observer.OnVoiceInteractionStatusChanged(state);
}

void VoiceInteractionController::NotifyVoiceInteractionSettingsEnabled(
    bool enabled) {
  voice_interaction_settings_enabled_ = enabled;
  for (auto& observer : voice_interaction_observers_)
    observer.OnVoiceInteractionSettingsEnabled(enabled);
}

void VoiceInteractionController::NotifyVoiceInteractionContextEnabled(
    bool enabled) {
  for (auto& observer : voice_interaction_observers_)
    observer.OnVoiceInteractionContextEnabled(enabled);
}

void VoiceInteractionController::NotifyVoiceInteractionSetupCompleted(
    bool completed) {
  voice_interaction_setup_completed_ = completed;
  for (auto& observer : voice_interaction_observers_)
    observer.OnVoiceInteractionSetupCompleted(completed);
}

}  // namespace ash
