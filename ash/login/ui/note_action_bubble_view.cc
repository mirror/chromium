// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/note_action_bubble_view.h"

#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/tray_action/tray_action.h"
#include "base/i18n/rtl.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animator.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/animation/ink_drop_painted_layer_delegates.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/masked_targeter_delegate.h"
#include "ui/views/view_targeter.h"

namespace ash {

namespace {

// The default note action bubble size.
constexpr int kSmallBubbleRadius = 48;

// The default note action bubble opacity.
constexpr float kSmallBubbleOpacity = 0.34;

// The note action bubble size when it's hovered/focused.
constexpr int kLargeBubbleRadius = 56;

// The note action bubble opacity when it's hovered/focused.
constexpr float kLargeBubbleOpacity = 0.46;

// The note action color - this is the value for all of the color's RGB
// components.
constexpr int kBubbleGrayColorShade = 0x9E;

// The note action icon size.
constexpr int kIconSize = 16;

// The note action icon padding from top and right bubble edges (assuming LTR
// layout).
constexpr int kIconPadding = 12;

// Layer delegate for painting a bubble of the specified radius and color to a
// layer. The bubble is painted as a quarter of a circle with the center in top
// right corner of the painted bounds.
// This is used to paint the bubble on the background view's layer.
class BubbleLayerDelegate : public views::BasePaintedLayerDelegate {
 public:
  BubbleLayerDelegate(SkColor color, int radius)
      : views::BasePaintedLayerDelegate(color), radius_(radius) {}

  ~BubbleLayerDelegate() override {}

  // views::BasePaintedLayerDelegate:
  gfx::RectF GetPaintedBounds() const override {
    return gfx::RectF(0, 0, radius_, radius_);
  }

  void OnPaintLayer(const ui::PaintContext& context) override {
    cc::PaintFlags flags;
    flags.setColor(color());
    flags.setAntiAlias(true);
    flags.setStyle(cc::PaintFlags::kFill_Style);

    ui::PaintRecorder recorder(context, gfx::Size(radius_, radius_));
    gfx::Canvas* canvas = recorder.canvas();
    canvas->Save();
    canvas->ClipRect(GetPaintedBounds());
    canvas->DrawCircle(base::i18n::IsRTL() ? GetPaintedBounds().origin()
                                           : GetPaintedBounds().top_right(),
                       radius_, flags);
    canvas->Restore();
  }

 private:
  // The radius of the circle.
  int radius_;

  DISALLOW_COPY_AND_ASSIGN(BubbleLayerDelegate);
};

}  // namespace

// The (background) view that paints and animatest the note action bubble.
class NoteActionBubbleView::BackgroundView : public NonAccessibleView {
 public:
  // NOTE: the background layer is set to the large bubble bounds and scaled
  // down when needed.
  BackgroundView()
      : background_layer_delegate_(SkColorSetRGB(kBubbleGrayColorShade,
                                                 kBubbleGrayColorShade,
                                                 kBubbleGrayColorShade),
                                   kLargeBubbleRadius) {
    // Painted to layer to provide background animations when the background
    // bubble is resized due to button changing its state (for example when
    // the button is hovered, the background should be expanded to
    // kLargeBubbleRadius).
    SetPaintToLayer();

    layer()->set_delegate(&background_layer_delegate_);
    layer()->SetMasksToBounds(true);
    layer()->SetFillsBoundsOpaquely(false);
    layer()->SetVisible(true);
    layer()->SetOpacity(opacity_);
  }

  ~BackgroundView() override = default;

  // Updates the bubble's opacity and radius. The bubble radius is updated
  // applying scale transform to the view's layout. Transformations are
  // animated.
  void SetBubbleRadiusAndOpacity(int target_radius, float opacity) {
    if (target_radius == bubble_radius_ && opacity_ == opacity)
      return;

    ui::ScopedLayerAnimationSettings settings(layer()->GetAnimator());
    settings.SetPreemptionStrategy(
        ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
    settings.SetTweenType(gfx::Tween::EASE_IN);

    gfx::Transform transform;
    if (target_radius != kLargeBubbleRadius) {
      // Move the buble to it's new origin before scaling the image - note that
      // in RTL layout, the origin remains the same - (0, 0) in local bounds.
      if (!base::i18n::IsRTL())
        transform.Translate(kLargeBubbleRadius - target_radius, 0);
      float scale = target_radius / static_cast<float>(kLargeBubbleRadius);
      transform.Scale(scale, scale);
    }

    layer()->SetTransform(transform);
    layer()->SetOpacity(opacity);

    bubble_radius_ = target_radius;
  }

 private:
  float opacity_ = 0;
  int bubble_radius_ = 0;

  BubbleLayerDelegate background_layer_delegate_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundView);
};

// The event target delegate used for the note action view. It matches the
// shape of the bubble with the provided radius.
class BubbleTargeterDelegate : public views::MaskedTargeterDelegate {
 public:
  explicit BubbleTargeterDelegate(int radius) : radius_(radius) {}
  ~BubbleTargeterDelegate() override = default;

  bool GetHitTestMask(gfx::Path* mask) const override {
    int center_x = base::i18n::IsRTL() ? 0 : radius_;
    mask->addCircle(SkIntToScalar(center_x), SkIntToScalar(0),
                    SkIntToScalar(radius_));
    return true;
  }

 private:
  int radius_;

  DISALLOW_COPY_AND_ASSIGN(BubbleTargeterDelegate);
};

// The action bubble foreground - an image button with actionable area matching
// the (small) bubble shape.
class NoteActionBubbleView::ActionButton : public views::ImageButton,
                                           public views::ButtonListener {
 public:
  explicit ActionButton(NoteActionBubbleView::BackgroundView* background)
      : views::ImageButton(this),
        background_(background),
        event_targeter_delegate_(kSmallBubbleRadius) {
    SetAccessibleName(
        l10n_util::GetStringUTF16(IDS_ASH_STYLUS_TOOLS_CREATE_NOTE_ACTION));
    SetImage(
        views::Button::STATE_NORMAL,
        CreateVectorIcon(kTrayActionNewLockScreenNoteIcon, kShelfIconColor));
    SetInkDropMode(InkDropMode::OFF);
    SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    SetFocusPainter(nullptr);
    EnableCanvasFlippingForRTLUI(true);
    SetPreferredSize(gfx::Size(kLargeBubbleRadius, kLargeBubbleRadius));
    SetEventTargeter(
        base::MakeUnique<views::ViewTargeter>(&event_targeter_delegate_));

    // Paint to layer because the background is painted to layer - if the button
    // was not painted to layer as well, the background would be painted over
    // it.
    SetPaintToLayer();
    layer()->SetMasksToBounds(true);
    layer()->SetFillsBoundsOpaquely(false);

    UpdateBubbleRadiusAndOpacity();
  }

  ~ActionButton() override = default;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    if (event.IsKeyEvent()) {
      Shell::Get()->tray_action()->RequestNewLockScreenNote(
          mojom::LockScreenNoteOrigin::kLockScreenButtonKeyboard);
    } else {
      Shell::Get()->tray_action()->RequestNewLockScreenNote(
          mojom::LockScreenNoteOrigin::kLockScreenButtonTap);
    }
  }

  // views::ImageButton:
  void PaintButtonContents(gfx::Canvas* canvas) override {
    canvas->Save();
    // Correctly position the icon image on the button's canvas.
    canvas->Translate(gfx::Vector2d(
        kLargeBubbleRadius - kIconSize - kIconPadding, kIconPadding));
    canvas->ClipRect(gfx::Rect(0, 0, kIconSize, kIconSize));

    views::ImageButton::PaintButtonContents(canvas);
    canvas->Restore();
  }

  // views::ImageButton:
  void StateChanged(ButtonState old_state) override {
    UpdateBubbleRadiusAndOpacity();
  }

  // views::ImageButton:
  void OnFocus() override { UpdateBubbleRadiusAndOpacity(); }

  // views::ImageButton:
  void OnBlur() override { UpdateBubbleRadiusAndOpacity(); }

 private:
  // Updates the background view size and opacity depending on the current note
  // action button state.
  void UpdateBubbleRadiusAndOpacity() {
    bool show_large_bubble =
        HasFocus() || state() == STATE_HOVERED || state() == STATE_PRESSED;
    background_->SetBubbleRadiusAndOpacity(
        show_large_bubble ? kLargeBubbleRadius : kSmallBubbleRadius,
        show_large_bubble ? kLargeBubbleOpacity : kSmallBubbleOpacity);
  }

  // The background view, which paints the note action bubble.
  NoteActionBubbleView::BackgroundView* background_;

  BubbleTargeterDelegate event_targeter_delegate_;

  DISALLOW_COPY_AND_ASSIGN(ActionButton);
};

NoteActionBubbleView::NoteActionBubbleView() : tray_action_observer_(this) {
  tray_action_observer_.Add(Shell::Get()->tray_action());

  SetLayoutManager(new views::FillLayout());

  background_ = new BackgroundView();
  AddChildView(background_);

  action_button_ = new ActionButton(background_);
  AddChildView(action_button_);

  UpdateVisibility(Shell::Get()->tray_action()->GetLockScreenNoteState());
}

NoteActionBubbleView::~NoteActionBubbleView() = default;

void NoteActionBubbleView::OnLockScreenNoteStateChanged(
    mojom::TrayActionState state) {
  UpdateVisibility(state);
}

void NoteActionBubbleView::UpdateVisibility(
    mojom::TrayActionState action_state) {
  SetVisible(action_state == mojom::TrayActionState::kAvailable);
}

}  // namespace ash
