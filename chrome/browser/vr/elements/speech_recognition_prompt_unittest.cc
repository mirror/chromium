// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/speech_recognition_prompt.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/animation_player.h"
#include "chrome/browser/vr/speech_recognizer.h"
#include "chrome/browser/vr/target_property.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

namespace {

static constexpr int kTestWidth = 256;

class TestSpeechRecognitionPrompt : public SpeechRecognitionPrompt {
 public:
  explicit TestSpeechRecognitionPrompt(int maximum_width)
      : SpeechRecognitionPrompt(maximum_width) {}
  ~TestSpeechRecognitionPrompt() override {}

  bool IsAnimatingProperty(int property) {
    return animation_player().IsAnimatingProperty(property);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestSpeechRecognitionPrompt);
};

}  // namespace

TEST(SpeechRecognitionPrompt, StartAndStopAnimation) {
  auto element = base::MakeUnique<TestSpeechRecognitionPrompt>(kTestWidth);

  element->OnSpeechRecognitionStateChanged(SPEECH_RECOGNITION_READY);
  EXPECT_TRUE(element->IsAnimatingProperty(CIRCLE_GROW));

  element->OnSpeechRecognitionStateChanged(SPEECH_RECOGNITION_END);
  EXPECT_FALSE(element->IsAnimatingProperty(CIRCLE_GROW));
}

}  // namespace vr
