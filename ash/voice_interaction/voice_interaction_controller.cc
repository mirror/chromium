// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/voice_interaction/voice_interaction_controller.h"
#include "base/bind.h"
#include "base/optional.h"
#include "components/signin/core/account_id/account_id.h"
#include "services/service_manager/public/cpp/connector.h"

namespace ash {

VoiceInteractionController::VoiceInteractionController(
    service_manager::Connector* connector)
    : binding_(this) {
  Shell::Get()->session_controller()->AddObserver(this);
  connector->BindInterface(chromeos::assistant::mojom::kAssistantServiceName,
                           &assistant_connector_);
}

VoiceInteractionController::~VoiceInteractionController() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

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

void VoiceInteractionController::OnActiveUserSessionChanged(
    const AccountId& account_id) {
#if defined(ENABLE_CROS_ASSISTANT)
  if (account_id.GetAccountType() != AccountType::GOOGLE) {
    assistant_connector_->OnActiveUserChanged(base::Optional<std::string>());
    return;
  }
  assistant_connector_->OnActiveUserChanged(
      base::Optional<std::string>(account_id.GetGaiaId()));
#endif
}

}  // namespace ash
