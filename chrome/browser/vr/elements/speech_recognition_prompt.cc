// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/speech_recognition_prompt.h"

#include "base/memory/ptr_util.h"
#include "cc/animation/keyframed_animation_curve.h"
#include "cc/animation/timing_function.h"
#include "cc/animation/transform_operations.h"
#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/animation_player.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "chrome/browser/vr/elements/vector_icon.h"
#include "chrome/browser/vr/elements/vector_icon_button_texture.h"
#include "chrome/browser/vr/speech_recognizer.h"
#include "chrome/browser/vr/target_property.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"

namespace vr {

namespace {

constexpr float kInnerCircleRadiusRatio = 0.5f;
constexpr float kIconScaleFactor = 0.3f;
constexpr SkColor kCircleColor = 0xFF4DB6AC;
constexpr float kStartProgress = 0.f;
constexpr float kEndProgress = 1.0f;
constexpr int kCircleGrowAnimationTimeMs = 1000;

}  // namespace

class SpeechRecognitionPromptTexture : public UiTexture {
 public:
  SpeechRecognitionPromptTexture() = default;
  ~SpeechRecognitionPromptTexture() override = default;

  void SetGrowingProgress(float progress) {
    growing_circle_progress_ = progress;
    set_dirty();
  }

 private:
  gfx::Size GetPreferredTextureSize(int maximum_width) const override {
    return gfx::Size(maximum_width, maximum_width);
  }

  gfx::SizeF GetDrawnSize() const override { return size_; }

  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override {
    DCHECK_EQ(texture_size.height(), texture_size.width());
    cc::SkiaPaintCanvas paint_canvas(sk_canvas);
    gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);
    gfx::Canvas* canvas = &gfx_canvas;

    size_.set_height(texture_size.height());
    size_.set_width(texture_size.width());

    float maximum_radius = size_.width() / 2;
    float inner_circle_radius = maximum_radius * kInnerCircleRadiusRatio;

    cc::PaintFlags flags;

    // Draw the growing circle. It starts from the size of inner circle and
    // grows to the maximum circle. Its alpha value decreases as it grows
    // and eventually becomes transparent once it is at full size.
    unsigned alpha = 255 * (1.0 - growing_circle_progress_);
    flags.setColor(SkColorSetA(kCircleColor, alpha));
    float radius =
        inner_circle_radius +
        (maximum_radius - inner_circle_radius) * growing_circle_progress_;
    canvas->DrawCircle(gfx::PointF(size_.width() / 2, size_.height() / 2),
                       radius, flags);

    // Draw inner circle
    flags.setColor(kCircleColor);
    canvas->DrawCircle(gfx::PointF(size_.width() / 2, size_.height() / 2),
                       inner_circle_radius, flags);

    float icon_size = size_.height() * kIconScaleFactor;
    float icon_corner_offset = (size_.height() - icon_size) / 2;
    VectorIcon::DrawVectorIcon(
        canvas, vector_icons::kMicrophoneIcon, icon_size,
        gfx::PointF(icon_corner_offset, icon_corner_offset), SK_ColorWHITE);
  }

  gfx::SizeF size_;
  float growing_circle_progress_ = 0.f;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognitionPromptTexture);
};

SpeechRecognitionPrompt::SpeechRecognitionPrompt(int preferred_width)
    : TexturedElement(preferred_width),
      texture_(base::MakeUnique<SpeechRecognitionPromptTexture>()) {}

SpeechRecognitionPrompt::~SpeechRecognitionPrompt() = default;

void SpeechRecognitionPrompt::OnSpeechRecognitionStateChanged(int new_state) {
  SpeechRecognitionState state = static_cast<SpeechRecognitionState>(new_state);
  switch (state) {
    case SPEECH_RECOGNITION_IN_SPEECH:
    case SPEECH_RECOGNITION_RECOGNIZING:
    case SPEECH_RECOGNITION_READY:
      StartListening();
      break;
    case SPEECH_RECOGNITION_OFF:
    case SPEECH_RECOGNITION_END:
      StopListening();
      break;
    case SPEECH_RECOGNITION_NETWORK_ERROR:
      break;
  }
}

void SpeechRecognitionPrompt::StartListening() {
  AddCircleGrowAnimation();
}

void SpeechRecognitionPrompt::StopListening() {
  animation_player().RemoveAnimations(CIRCLE_GROW);
  texture_->SetGrowingProgress(0.0f);
}

void SpeechRecognitionPrompt::AddCircleGrowAnimation() {
  std::unique_ptr<cc::KeyframedFloatAnimationCurve> curve(
      cc::KeyframedFloatAnimationCurve::Create());

  curve->AddKeyframe(
      cc::FloatKeyframe::Create(base::TimeDelta(), kStartProgress, nullptr));
  curve->AddKeyframe(cc::FloatKeyframe::Create(
      base::TimeDelta::FromMilliseconds(kCircleGrowAnimationTimeMs),
      kEndProgress, nullptr));

  std::unique_ptr<cc::Animation> animation(cc::Animation::Create(
      std::move(curve), AnimationPlayer::GetNextAnimationId(),
      AnimationPlayer::GetNextGroupId(), CIRCLE_GROW));
  animation->set_iterations(-1);
  AddAnimation(std::move(animation));
}

UiTexture* SpeechRecognitionPrompt::GetTexture() const {
  return texture_.get();
}

void SpeechRecognitionPrompt::NotifyClientFloatAnimated(
    float value,
    int target_property_id,
    cc::Animation* animation) {
  if (target_property_id == CIRCLE_GROW) {
    texture_->SetGrowingProgress(value);
    return;
  }
  TexturedElement::NotifyClientFloatAnimated(value, target_property_id,
                                             animation);
}

}  // namespace vr
