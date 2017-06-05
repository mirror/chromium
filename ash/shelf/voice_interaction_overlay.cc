// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/voice_interaction_overlay.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "ash/public/cpp/shelf_types.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shelf/app_list_button.h"
#include "ash/shelf/ink_drop_button_listener.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "chromeos/chromeos_switches.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/app_list/presenter/app_list.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/layer_animation_element.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/layer_animation_sequence.h"
#include "ui/compositor/paint_context.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/painter.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {
constexpr int kFullExpandDurationMs = 650;
constexpr int kFullRetractDurationMs = 300;
constexpr int kFullBurstDurationMs = 200;

constexpr float kRippleCircleInitRadiusDp = 40.f;
constexpr float kRippleCircleStartRadiusDp = 1.f;
constexpr float kRippleCircleRadiusDp = 77.f;
constexpr float kRippleCircleBurstRadiusDp = 96.f;
constexpr SkColor kRippleColor = SK_ColorWHITE;
constexpr int kRippleExpandDurationMs = 400;
constexpr int kRippleOpacityDurationMs = 100;
constexpr int kRippleOpacityRetractDurationMs = 200;
constexpr float kRippleOpacity = 0.1f;

constexpr float kIconInitSizeDp = 24.f;
constexpr float kIconStartSizeDp = 4.f;
constexpr float kIconSizeDp = 24.f;
constexpr float kIconOffsetDp = 56.f;
constexpr float kIconOpacity = 1.f;
constexpr int kIconBurstDurationMs = 100;

constexpr float kBackgroundInitSizeDp = 48.f;
constexpr float kBackgroundStartSizeDp = 10.f;
constexpr float kBackgroundSizeDp = 48.f;
constexpr int kBackgroundStartDelayMs = 100;
constexpr int kBackgroundOpacityDurationMs = 200;
constexpr float kBackgroundShadowSizeDp = 12;

class CircleImageSource : public gfx::CanvasImageSource {
 public:
  CircleImageSource(float radius, float padding)
      : CanvasImageSource(
            gfx::Size((radius + padding) * 2, (radius + padding) * 2),
            false),
        radius_(radius),
        padding_(padding) {}
  ~CircleImageSource() override {}

 private:
  void Draw(gfx::Canvas* canvas) override {
    cc::PaintFlags flags;
    flags.setColor(SK_ColorWHITE);
    flags.setAntiAlias(true);
    flags.setStyle(cc::PaintFlags::kFill_Style);
    canvas->DrawCircle({radius_ + padding_, radius_ + padding_}, radius_,
                       flags);
  }

  float radius_;
  float padding_;

  DISALLOW_COPY_AND_ASSIGN(CircleImageSource);
};
}  // namespace

class VoiceInteractionIconBackground : public ui::Layer, ui::LayerDelegate {
 public:
  VoiceInteractionIconBackground() : Layer(ui::LAYER_NOT_DRAWN) {
    set_name("VoiceInteractionOverlay:BACKGROUND_LAYER");
    SetBounds({0, 0, kBackgroundInitSizeDp, kBackgroundInitSizeDp});
    SetFillsBoundsOpaquely(false);
    SetMasksToBounds(false);

    shadow_layer_.set_delegate(this);
    shadow_layer_.SetFillsBoundsOpaquely(false);
    shadow_layer_.SetBounds(
        {-kBackgroundShadowSizeDp, -kBackgroundShadowSizeDp,
         kBackgroundInitSizeDp + kBackgroundShadowSizeDp * 2,
         kBackgroundInitSizeDp + kBackgroundShadowSizeDp * 2});
    Add(&shadow_layer_);

    shadow_values_ = {
        1, gfx::ShadowValue(gfx::Vector2d(0, 0), kBackgroundShadowSizeDp,
                            SkColorSetARGB(0x54, 0, 0, 0))};
  }
  ~VoiceInteractionIconBackground() override{};

 private:
  void OnPaintLayer(const ui::PaintContext& context) override {
    // Radius is based on the parent layer size, the shadow layer is expanded
    // to make room for the shadow.
    float radius = size().width() / 2.f;
    CircleImageSource* circle_image =
        new CircleImageSource(radius, kBackgroundShadowSizeDp);
    gfx::ImageSkia background_with_shadow =
        gfx::ImageSkiaOperations::CreateImageWithDropShadow(
            gfx::ImageSkia(circle_image, circle_image->size()), shadow_values_);

    ui::PaintRecorder recorder(context, background_with_shadow.size());
    gfx::Canvas* canvas = recorder.canvas();
    float shadow_offset =
        (shadow_layer_.size().width() - background_with_shadow.width()) / 2.f;
    canvas->DrawImageInt(background_with_shadow, shadow_offset, shadow_offset);
  }

  void OnDelegatedFrameDamage(const gfx::Rect& damage_rect_in_dip) override {}

  void OnDeviceScaleFactorChanged(float device_scale_factor) override {}

  gfx::ShadowValues shadow_values_;

  ui::Layer shadow_layer_;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionIconBackground);
};

class VoiceInteractionIcon : public ui::Layer, ui::LayerDelegate {
 public:
  VoiceInteractionIcon() {
    set_name("VoiceInteractionOverlay:ICON_LAYER");
    SetBounds({0, 0, kIconInitSizeDp, kIconInitSizeDp});
    SetFillsBoundsOpaquely(false);
    SetMasksToBounds(false);
    set_delegate(this);
  }

 private:
  void OnPaintLayer(const ui::PaintContext& context) override {
    ui::PaintRecorder recorder(context, size());
    gfx::PaintVectorIcon(recorder.canvas(), kShelfVoiceInteractionIcon,
                         kIconInitSizeDp, 0);
  }

  void OnDelegatedFrameDamage(const gfx::Rect& damage_rect_in_dip) override {}

  void OnDeviceScaleFactorChanged(float device_scale_factor) override {}

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionIcon);
};

VoiceInteractionOverlay::VoiceInteractionOverlay(AppListButton* host_view)
    : ripple_layer_(new ui::Layer()),
      icon_layer_(new VoiceInteractionIcon()),
      background_layer_(new VoiceInteractionIconBackground()),
      host_view_(host_view),
      is_bursting_(false),
      circle_layer_delegate_(kRippleColor, kRippleCircleInitRadiusDp) {
  SetPaintToLayer(ui::LAYER_NOT_DRAWN);
  layer()->set_name("VoiceInteractionOverlay:ROOT_LAYER");
  layer()->SetMasksToBounds(false);

  ripple_layer_->SetBounds(gfx::Rect(0, 0, kRippleCircleInitRadiusDp * 2,
                                     kRippleCircleInitRadiusDp * 2));
  ripple_layer_->set_delegate(&circle_layer_delegate_);
  ripple_layer_->SetFillsBoundsOpaquely(false);
  ripple_layer_->SetMasksToBounds(true);
  ripple_layer_->set_name("VoiceInteractionOverlay:PAINTED_LAYER");
  layer()->Add(ripple_layer_.get());

  layer()->Add(background_layer_.get());

  layer()->Add(icon_layer_.get());
}

VoiceInteractionOverlay::~VoiceInteractionOverlay() {}

void VoiceInteractionOverlay::StartAnimation() {
  is_bursting_ = false;
  SetVisible(true);

  // Setup ripple initial state.
  ripple_layer_->SetOpacity(0);

  SkMScalar scale_factor =
      kRippleCircleStartRadiusDp / kRippleCircleInitRadiusDp;
  std::unique_ptr<gfx::Transform> transform(new gfx::Transform());

  gfx::Point center = host_view_->GetCenterPoint();
  transform->Translate({center.x() - kRippleCircleStartRadiusDp,
                        center.y() - kRippleCircleStartRadiusDp});
  transform->Scale(scale_factor, scale_factor);
  ripple_layer_->SetTransform(*transform.get());

  // Setup ripple animations.
  scale_factor = kRippleCircleRadiusDp / kRippleCircleInitRadiusDp;
  transform.reset(new gfx::Transform());
  transform->Translate(
      {center.x() - kRippleCircleRadiusDp, center.y() - kRippleCircleRadiusDp});
  transform->Scale(scale_factor, scale_factor);

  std::unique_ptr<ui::LayerAnimationElement> animation_element =
      ui::LayerAnimationElement::CreateTransformElement(
          *transform.get(),
          base::TimeDelta::FromMilliseconds(kRippleExpandDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  ui::LayerAnimationSequence* sequence =
      new ui::LayerAnimationSequence(std::move(animation_element));

  ripple_layer_->GetAnimator()->StartAnimation(sequence);

  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      kRippleOpacity,
      base::TimeDelta::FromMilliseconds(kRippleOpacityDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  sequence = new ui::LayerAnimationSequence(std::move(animation_element));

  ripple_layer_->GetAnimator()->StartAnimation(sequence);

  // We need to determine the animation direction based on shelf alignment.
  ShelfAlignment alignment =
      Shelf::ForWindow(host_view_->GetWidget()->GetNativeWindow())->alignment();

  // Setup icon initial state.
  icon_layer_->SetOpacity(0);
  transform.reset(new gfx::Transform());

  transform->Translate({center.x() - kIconStartSizeDp / 2.f,
                        center.y() - kIconStartSizeDp / 2.f});

  scale_factor = kIconStartSizeDp / kIconInitSizeDp;
  transform->Scale(scale_factor, scale_factor);
  icon_layer_->SetTransform(*transform.get());

  // Setup icon animation.
  scale_factor = kIconSizeDp / kIconInitSizeDp;
  transform.reset(new gfx::Transform());
  if (alignment == SHELF_ALIGNMENT_BOTTOM ||
      alignment == SHELF_ALIGNMENT_BOTTOM_LOCKED) {
    transform->Translate({center.x() - kIconSizeDp / 2 + kIconOffsetDp,
                          center.y() - kIconSizeDp / 2 - kIconOffsetDp});
  } else if (alignment == SHELF_ALIGNMENT_RIGHT) {
    transform->Translate({center.x() - kIconSizeDp / 2 - kIconOffsetDp,
                          center.y() - kIconSizeDp / 2 + kIconOffsetDp});
  } else {
    DCHECK_EQ(alignment, SHELF_ALIGNMENT_LEFT);
    transform->Translate({center.x() - kIconSizeDp / 2 + kIconOffsetDp,
                          center.y() - kIconSizeDp / 2 + kIconOffsetDp});
  }
  transform->Scale(scale_factor, scale_factor);

  animation_element = ui::LayerAnimationElement::CreateTransformElement(
      *transform.get(),
      base::TimeDelta::FromMilliseconds(kFullExpandDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  icon_layer_->GetAnimator()->StartAnimation(
      new ui::LayerAnimationSequence(std::move(animation_element)));

  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      kIconOpacity, base::TimeDelta::FromMilliseconds(kFullExpandDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  sequence = new ui::LayerAnimationSequence(std::move(animation_element));
  icon_layer_->GetAnimator()->StartAnimation(sequence);

  // Setup background initial state.
  background_layer_->SetVisible(false);
  background_layer_->SetOpacity(0);

  transform.reset(new gfx::Transform());

  transform->Translate({center.x() - kBackgroundStartSizeDp / 2.f,
                        center.y() - kBackgroundStartSizeDp / 2.f});

  scale_factor = kBackgroundStartSizeDp / kBackgroundInitSizeDp;
  transform->Scale(scale_factor, scale_factor);
  background_layer_->SetTransform(*transform.get());

  // Setup background animation.
  scale_factor = kBackgroundSizeDp / kBackgroundInitSizeDp;
  transform.reset(new gfx::Transform());
  if (alignment == SHELF_ALIGNMENT_BOTTOM ||
      alignment == SHELF_ALIGNMENT_BOTTOM_LOCKED) {
    transform->Translate({center.x() - kBackgroundSizeDp / 2 + kIconOffsetDp,
                          center.y() - kBackgroundSizeDp / 2 - kIconOffsetDp});
  } else if (alignment == SHELF_ALIGNMENT_RIGHT) {
    transform->Translate({center.x() - kBackgroundSizeDp / 2 - kIconOffsetDp,
                          center.y() - kBackgroundSizeDp / 2 + kIconOffsetDp});
  } else {
    DCHECK_EQ(alignment, SHELF_ALIGNMENT_LEFT);
    transform->Translate({center.x() - kBackgroundSizeDp / 2 + kIconOffsetDp,
                          center.y() - kBackgroundSizeDp / 2 + kIconOffsetDp});
  }
  transform->Scale(scale_factor, scale_factor);

  animation_element = ui::LayerAnimationElement::CreateTransformElement(
      *transform.get(),
      base::TimeDelta::FromMilliseconds(kFullExpandDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  background_layer_->GetAnimator()->StartAnimation(
      new ui::LayerAnimationSequence(std::move(animation_element)));

  animation_element = ui::LayerAnimationElement::CreateVisibilityElement(
      true, base::TimeDelta::FromMilliseconds(kBackgroundStartDelayMs));

  sequence = new ui::LayerAnimationSequence(std::move(animation_element));

  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      1, base::TimeDelta::FromMilliseconds(kBackgroundOpacityDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  sequence->AddElement(std::move(animation_element));

  background_layer_->GetAnimator()->StartAnimation(sequence);
}

void VoiceInteractionOverlay::BurstAnimation() {
  is_bursting_ = true;

  gfx::Point center = host_view_->GetCenterPoint();

  // Abort the current animation
  ripple_layer_->GetAnimator()->AbortAllAnimations();
  icon_layer_->GetAnimator()->AbortAllAnimations();
  background_layer_->GetAnimator()->AbortAllAnimations();

  // Setup ripple animations.
  SkMScalar scale_factor =
      kRippleCircleBurstRadiusDp / kRippleCircleInitRadiusDp;
  std::unique_ptr<gfx::Transform> transform(new gfx::Transform());
  transform->Translate({center.x() - kRippleCircleBurstRadiusDp,
                        center.y() - kRippleCircleBurstRadiusDp});
  transform->Scale(scale_factor, scale_factor);

  std::unique_ptr<ui::LayerAnimationElement> animation_element =
      ui::LayerAnimationElement::CreateTransformElement(
          *transform.get(),
          base::TimeDelta::FromMilliseconds(kFullBurstDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  ui::LayerAnimationSequence* sequence =
      new ui::LayerAnimationSequence(std::move(animation_element));

  ripple_layer_->GetAnimator()->StartAnimation(sequence);

  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      0, base::TimeDelta::FromMilliseconds(kFullBurstDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  sequence = new ui::LayerAnimationSequence(std::move(animation_element));

  ripple_layer_->GetAnimator()->StartAnimation(sequence);

  // Setup icon animation.
  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      0, base::TimeDelta::FromMilliseconds(kIconBurstDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  sequence = new ui::LayerAnimationSequence(std::move(animation_element));

  icon_layer_->GetAnimator()->StartAnimation(sequence);

  // Setup background animation.
  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      0, base::TimeDelta::FromMilliseconds(kIconBurstDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  sequence = new ui::LayerAnimationSequence(std::move(animation_element));

  background_layer_->GetAnimator()->StartAnimation(sequence);
}

void VoiceInteractionOverlay::EndAnimation() {
  if (is_bursting_) {
    // Too late, user action already fired, we have to finish what's started.
    return;
  }

  // Abort the current animation
  ripple_layer_->GetAnimator()->AbortAllAnimations();
  icon_layer_->GetAnimator()->AbortAllAnimations();
  background_layer_->GetAnimator()->AbortAllAnimations();

  // Play reverse animation
  // Setup ripple animations.
  SkMScalar scale_factor =
      kRippleCircleStartRadiusDp / kRippleCircleInitRadiusDp;
  std::unique_ptr<gfx::Transform> transform(new gfx::Transform());

  gfx::Point center = host_view_->GetCenterPoint();
  transform->Translate({center.x() - kRippleCircleStartRadiusDp,
                        center.y() - kRippleCircleStartRadiusDp});
  transform->Scale(scale_factor, scale_factor);

  std::unique_ptr<ui::LayerAnimationElement> animation_element =
      ui::LayerAnimationElement::CreateTransformElement(
          *transform.get(),
          base::TimeDelta::FromMilliseconds(kFullRetractDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  ripple_layer_->GetAnimator()->StartAnimation(
      new ui::LayerAnimationSequence(std::move(animation_element)));

  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      0, base::TimeDelta::FromMilliseconds(kRippleOpacityRetractDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  ripple_layer_->GetAnimator()->StartAnimation(
      new ui::LayerAnimationSequence(std::move(animation_element)));

  // Setup icon animation.
  transform.reset(new gfx::Transform());

  transform->Translate({center.x() - kIconStartSizeDp / 2.f,
                        center.y() - kIconStartSizeDp / 2.f});

  scale_factor = kIconStartSizeDp / kIconInitSizeDp;
  transform->Scale(scale_factor, scale_factor);

  animation_element = ui::LayerAnimationElement::CreateTransformElement(
      *transform.get(),
      base::TimeDelta::FromMilliseconds(kFullRetractDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  icon_layer_->GetAnimator()->StartAnimation(
      new ui::LayerAnimationSequence(std::move(animation_element)));

  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      0, base::TimeDelta::FromMilliseconds(kFullRetractDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  ui::LayerAnimationSequence* sequence =
      new ui::LayerAnimationSequence(std::move(animation_element));
  icon_layer_->GetAnimator()->StartAnimation(sequence);

  // Setup background animation.
  transform.reset(new gfx::Transform());

  transform->Translate({center.x() - kBackgroundStartSizeDp / 2.f,
                        center.y() - kBackgroundStartSizeDp / 2.f});

  scale_factor = kBackgroundStartSizeDp / kBackgroundInitSizeDp;
  transform->Scale(scale_factor, scale_factor);

  animation_element = ui::LayerAnimationElement::CreateTransformElement(
      *transform.get(),
      base::TimeDelta::FromMilliseconds(kFullRetractDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  background_layer_->GetAnimator()->StartAnimation(
      new ui::LayerAnimationSequence(std::move(animation_element)));

  animation_element = ui::LayerAnimationElement::CreateOpacityElement(
      0, base::TimeDelta::FromMilliseconds(kFullRetractDurationMs));
  animation_element->set_tween_type(gfx::Tween::LINEAR_OUT_SLOW_IN);
  sequence = new ui::LayerAnimationSequence(std::move(animation_element));

  background_layer_->GetAnimator()->StartAnimation(sequence);
}

}  // namespace ash
