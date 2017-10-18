// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/badged_profile_photo.h"

#include "base/macros.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "ui/compositor/clip_recorder.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/paint_info.h"
#include "ui/views/view.h"

namespace {

// Helpers --------------------------------------------------------------------

const int kProfileBadgeSize = 24;
const int kSupervisedIconBadgeSize = 22;
const int kImageSideLength = 40;

}  // namespace

// BadgedProfilePhoto -------------------------------------------------

BadgedProfilePhoto::BadgedProfilePhoto(views::ButtonListener* listener,
                                       BadgeType badge_type,
                                       const gfx::Image& icon)
    : views::LabelButton(listener, base::string16()),
      photo_overlay_(nullptr),
      badge_type_(badge_type) {
  set_can_process_events_within_subtree(false);
  gfx::Image image = profiles::GetSizedAvatarIcon(icon, true, kImageSideLength,
                                                  kImageSideLength);
  SetImage(views::LabelButton::STATE_NORMAL, *image.ToImageSkia());
  SetBorder(views::NullBorder());
  SetMinSize(gfx::Size(GetPreferredSize().width() + kBadgeSpacing,
                       GetPreferredSize().height() + kBadgeSpacing +
                           ChromeLayoutProvider::Get()->GetDistanceMetric(
                               DISTANCE_RELATED_CONTROL_VERTICAL_SMALL)));
  SetEnabled(false);
}

void BadgedProfilePhoto::PaintChildren(const views::PaintInfo& paint_info) {
  {
    // Display any children (the "change photo" overlay) as a circle.
    ui::ClipRecorder clip_recorder(paint_info.context());
    gfx::Rect clip_bounds = image()->GetMirroredBounds();
    gfx::Path clip_mask;
    clip_mask.addCircle(clip_bounds.x() + clip_bounds.width() / 2,
                        clip_bounds.y() + clip_bounds.height() / 2,
                        clip_bounds.width() / 2);
    clip_recorder.ClipPathWithAntiAliasing(clip_mask);
    View::PaintChildren(paint_info);
  }

  ui::PaintRecorder paint_recorder(
      paint_info.context(), gfx::Size(kProfileBadgeSize, kProfileBadgeSize));
  gfx::Canvas* canvas = paint_recorder.canvas();
  if (badge_type_ != BADGE_TYPE_NONE) {
    gfx::Rect bounds(0, 0, kProfileBadgeSize, kProfileBadgeSize);
    int badge_offset = kImageSideLength + kBadgeSpacing - kProfileBadgeSize;
    gfx::Vector2d badge_offset_vector = gfx::Vector2d(
        GetMirroredXWithWidthInView(badge_offset, kProfileBadgeSize),
        badge_offset + ChromeLayoutProvider::Get()->GetDistanceMetric(
                           DISTANCE_RELATED_CONTROL_VERTICAL_SMALL));

    gfx::Point center_point = bounds.CenterPoint() + badge_offset_vector;

    // Paint the circular background.
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(GetNativeTheme()->GetSystemColor(
        ui::NativeTheme::kColorId_BubbleBackground));
    canvas->DrawCircle(center_point, kProfileBadgeSize / 2, flags);

    const gfx::VectorIcon* icon = nullptr;

    switch (badge_type_) {
      case BADGE_TYPE_SUPERVISED:
        icon = &kSupervisorAccountCircleIcon;
        break;
      case BADGE_TYPE_CHILD:
        icon = &kAccountChildCircleIcon;
        break;
      case BADGE_TYPE_NONE:
        NOTREACHED();
        break;
    }

    // Paint the badge icon.
    int offset = (kProfileBadgeSize - kSupervisedIconBadgeSize) / 2;
    canvas->Translate(badge_offset_vector + gfx::Vector2d(offset, offset));
    gfx::PaintVectorIcon(canvas, *icon, kSupervisedIconBadgeSize,
                         gfx::kChromeIconGrey);
  }
}

void BadgedProfilePhoto::StateChanged(ButtonState old_state) {
  if (photo_overlay_) {
    photo_overlay_->SetVisible(state() == STATE_PRESSED ||
                               state() == STATE_HOVERED || HasFocus());
  }
}

void BadgedProfilePhoto::OnFocus() {
  views::LabelButton::OnFocus();
  if (photo_overlay_)
    photo_overlay_->SetVisible(true);
}

void BadgedProfilePhoto::OnBlur() {
  views::LabelButton::OnBlur();
  // Don't hide the overlay if it's being shown as a result of a mouseover.
  if (photo_overlay_ && state() != STATE_HOVERED)
    photo_overlay_->SetVisible(false);
}
