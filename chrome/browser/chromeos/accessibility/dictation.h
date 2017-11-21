// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#ifndef CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_DICTATION_H_
#define CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_DICTATION_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/app_list/speech_recognizer.h"
#include "chrome/browser/ui/app_list/speech_recognizer_delegate.h"
#include "content/public/browser/speech_recognition_session_preamble.h"

class Profile;

namespace app_list {
class SpeechRecognizer;
}  // namespace app_list

namespace chromeos {

class Dictation : public app_list::SpeechRecognizerDelegate {
 public:
  explicit Dictation(Profile* profile);
  ~Dictation() override;

  // User initiated dictation.
  void OnToggleDictation();

 private:
  // app_list::SpeechRecognizerDelegate:
  void OnSpeechResult(const base::string16& query, bool is_final) override;
  void OnSpeechSoundLevelChanged(int16_t level) override {}
  void OnSpeechRecognitionStateChanged(
      app_list::SpeechRecognitionState new_state) override;
  void GetSpeechAuthParameters(std::string* auth_scope,
                               std::string* auth_token) override {}

  std::unique_ptr<app_list::SpeechRecognizer> speech_recognizer_;

  Profile* profile_;

  base::WeakPtrFactory<Dictation> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Dictation);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ACCESSIBILITY_DICTATION_H_
