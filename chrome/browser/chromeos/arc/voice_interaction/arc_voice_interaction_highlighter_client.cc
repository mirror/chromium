// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_highlighter_client.h"

#include "ash/highlighter/highlighter_controller.h"
#include "ash/highlighter/highlighter_selection_observer.h"
#include "ash/shell.h"
#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_framework_service.h"

namespace arc {

namespace {
constexpr int kSelectionReportDelayMs = 600;
}

ArcVoiceInteractionHighlighterClient::ArcVoiceInteractionHighlighterClient(
    ArcVoiceInteractionFrameworkService* service)
    : service_(service) {
  ash::Shell::Get()->highlighter_controller()->SetObserver(this);
}

ArcVoiceInteractionHighlighterClient::~ArcVoiceInteractionHighlighterClient() {
  if (ash::Shell::HasInstance() &&
      ash::Shell::Get()->highlighter_controller()) {
    ash::Shell::Get()->highlighter_controller()->SetObserver(nullptr);
  }
}

void ArcVoiceInteractionHighlighterClient::HandleSelection(
    const gfx::Rect& rect) {
  // Delay the actual voice interaction service invocation for better
  // visual synchronization with the metalayer animation.
  delay_timer_ = base::MakeUnique<base::Timer>(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kSelectionReportDelayMs),
      base::Bind(&ArcVoiceInteractionHighlighterClient::ReportSelection,
                 base::Unretained(this), rect),
      false /* not repeating */);
  delay_timer_->Reset();
}

void ArcVoiceInteractionHighlighterClient::HandleFailedSelection() {}

void ArcVoiceInteractionHighlighterClient::HandleEnabledStateChange(
    bool enabled) {
  // ArcVoiceInteractionFrameworkService::HideMetalayer() causes the container
  // to show a toast-like prompt. This toast is redundant and causes
  // unnecessary flicker if the full voice interaction UI is about to be
  // displayed soon. |start_session_pending| is a good signal that the session
  // is about to start, but it is not guaranteed: 1) The user might re-enter the
  // metalayer mode before the timer fires.
  //    In this case the container will keep showing the prompt for the
  //    metalayer mode.
  // 2) The session might fail to start due to a peculiar combination of
  //    failures on the way to the voice interaction UI. This is an open
  //    problem.
  if (enabled)
    service_->ShowMetalayer();
  else if (!start_session_pending())
    service_->HideMetalayer();
}

void ArcVoiceInteractionHighlighterClient::ReportSelection(
    const gfx::Rect& rect) {
  service_->StartSessionFromUserInteraction(rect);
}

}  // namespace arc
