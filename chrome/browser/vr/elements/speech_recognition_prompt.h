// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_SPEECH_RECOGNITION_PROMPT_H_
#define CHROME_BROWSER_VR_ELEMENTS_SPEECH_RECOGNITION_PROMPT_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/vr/elements/textured_element.h"

namespace vr {

class SpeechRecognitionPromptTexture;

class SpeechRecognitionPrompt : public TexturedElement {
 public:
  explicit SpeechRecognitionPrompt(int preferred_width);
  ~SpeechRecognitionPrompt() override;

  void OnSpeechRecognitionStateChanged(int new_state);

 private:
  void StartListening();
  void StopListening();

  void AddCircleGrowAnimation();
  void NotifyClientFloatAnimated(float value,
                                 int target_property_id,
                                 cc::Animation* animation) override;

  UiTexture* GetTexture() const override;

  std::unique_ptr<SpeechRecognitionPromptTexture> texture_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognitionPrompt);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_SPEECH_RECOGNITION_PROMPT_H_
