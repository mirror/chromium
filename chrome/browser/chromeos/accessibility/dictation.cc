// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/accessibility/dictation.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/speech/speech_recognizer.h"
#include "chrome/common/pref_names.h"
#include "chromeos/audio/chromeos_sounds.h"
#include "components/prefs/pref_service.h"
#include "media/audio/sounds/sounds_manager.h"
#include "ui/base/ime/ime_bridge.h"
#include "ui/base/ime/ime_input_context_handler_interface.h"

namespace {

const char kDefaultProfileLocale[] = "en-US";

std::string GetUserLocale(Profile* profile) {
  const std::string user_locale =
      profile->GetPrefs()->GetString(prefs::kApplicationLocale);

  if (!user_locale.length())
    return kDefaultProfileLocale;

  return user_locale;
}

}  // namespace

Dictation::Dictation(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {}

Dictation::~Dictation() = default;

void Dictation::OnToggleDictation() {
  if (speech_recognizer_) {
    media::SoundsManager::Get()->Play(chromeos::SOUND_EXIT_SCREEN);
    speech_recognizer_.reset();
    return;
  }

  speech_recognizer_ = std::make_unique<SpeechRecognizer>(
      weak_ptr_factory_.GetWeakPtr(), profile_->GetRequestContext(),
      GetUserLocale(profile_));
  speech_recognizer_->Start(nullptr);
}

void Dictation::OnSpeechResult(const base::string16& query, bool is_final) {
  if (!is_final)
    return;

  ui::IMEInputContextHandlerInterface* input_context =
      ui::IMEBridge::Get()->GetInputContextHandler();
  if (input_context)
    input_context->CommitText(base::UTF16ToASCII(query));

  speech_recognizer_.reset();
}

void Dictation::OnSpeechSoundLevelChanged(int16_t level) {}

void Dictation::OnSpeechRecognitionStateChanged(
    SpeechRecognizerState new_state) {
  if (new_state == SPEECH_RECOGNIZER_RECOGNIZING)
    media::SoundsManager::Get()->Play(chromeos::SOUND_ENTER_SCREEN);
}

void Dictation::GetSpeechAuthParameters(std::string* auth_scope,
                                        std::string* auth_token) {}
