// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/voice_interaction/voice_interaction_controller.h"

#include <utility>

#include "services/service_manager/public/cpp/connector.h"

namespace ash {

VoiceInteractionController::VoiceInteractionController(
    service_manager::Connector* connector)
    : binding_(this) {
  connector->BindInterface(mojom::kAssistantConnectorServiceName,
                           &assistant_connector_);
}

VoiceInteractionController::~VoiceInteractionController() = default;

void VoiceInteractionController::BindRequest(
    mojom::VoiceInteractionControllerRequest request) {
  binding_.Bind(std::move(request));
}

void VoiceInteractionController::AddObserver(
    VoiceInteractionObserver* observer) {
  observers_.AddObserver(observer);
}

void VoiceInteractionController::RemoveObserver(
    VoiceInteractionObserver* observer) {
  observers_.RemoveObserver(observer);
}

void VoiceInteractionController::NotifyStatusChanged(
    mojom::VoiceInteractionState state) {
  voice_interaction_state_ = state;
  for (auto& observer : observers_)
    observer.OnVoiceInteractionStatusChanged(state);
}

void VoiceInteractionController::NotifySettingsEnabled(bool enabled) {
  settings_enabled_ = enabled;
  for (auto& observer : observers_)
    observer.OnVoiceInteractionSettingsEnabled(enabled);
}

void VoiceInteractionController::NotifyContextEnabled(bool enabled) {
  for (auto& observer : observers_)
    observer.OnVoiceInteractionContextEnabled(enabled);
}

void VoiceInteractionController::NotifySetupCompleted(bool completed) {
  setup_completed_ = completed;
  for (auto& observer : observers_)
    observer.OnVoiceInteractionSetupCompleted(completed);
}

void VoiceInteractionController::NotifyFeatureAllowed(
    mojom::AssistantAllowedState state) {
  allowed_state_ = state;
  for (auto& observer : observers_)
    observer.OnAssistantFeatureAllowedChanged(state);
}

void VoiceInteractionController::ShowCard() {
  assistant_connector_->ShowAssistantCard();
}

void VoiceInteractionController::SendTextQuery(const std::string& query) {
  assistant_connector_->SendTextQuery(query);
}

}  // namespace ash
