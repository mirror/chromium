// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/badged_profile_photo.h"

#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/image_view.h"

namespace {

// Helpers --------------------------------------------------------------------

constexpr int kBadgeIconSize = 16;
constexpr int kBadgeBorderWidth = 2;
// Width/Height of the current profile photo.
constexpr int kImageSize = 40;
// Width and Height of the badged profile photo. Doesn't include the badge
// border to the right and to the bottom.
constexpr int kBadgedProfilePhotoWidth =
    kImageSize + BadgedProfilePhoto::kBadgeSpacing;
constexpr int kBadgedProfilePhotoHeight = kImageSize;

// A custom ImageView that removes the part where the badge will be placed
// including the (transparent) border.
class CustomImageView : public views::ImageView {
 public:
  CustomImageView() = default;

 private:
  // views::ImageView:
  void OnPaint(gfx::Canvas* canvas) override;

  DISALLOW_COPY_AND_ASSIGN(CustomImageView);
};

void CustomImageView::OnPaint(gfx::Canvas* canvas) {
  // Remove the part of the ImageView that contains the badge.
  gfx::Path mask;
  mask.addCircle(kBadgedProfilePhotoWidth - kBadgeIconSize / 2,
                 kBadgedProfilePhotoHeight - kBadgeIconSize / 2,
                 kBadgeIconSize / 2 + kBadgeBorderWidth);
  mask.toggleInverseFillType();
  canvas->ClipPath(mask, true);
  ImageView::OnPaint(canvas);
}

}  // namespace

// BadgedProfilePhoto -------------------------------------------------

BadgedProfilePhoto::BadgedProfilePhoto(BadgeType badge_type,
                                       const gfx::Image& icon) {
  set_can_process_events_within_subtree(false);

  // Create and add image view for profile icon.
  gfx::Image profile_icon_image = profiles::GetSizedAvatarIcon(
      icon, true, kImageSize, kImageSize, profiles::SHAPE_CIRCLE);
  views::ImageView* profile_icon_view =
      (badge_type == BADGE_TYPE_NONE ? new views::ImageView()
                                     : new CustomImageView());
  profile_icon_view->SetImage(*profile_icon_image.ToImageSkia());
  profile_icon_view->SizeToPreferredSize();
  AddChildView(profile_icon_view);
  profile_icon_view->SetPosition(gfx::Point(0, 0));

  if (badge_type != BADGE_TYPE_NONE) {
    const gfx::VectorIcon* badge_icon = nullptr;
    switch (badge_type) {
      case BADGE_TYPE_SUPERVISOR:
        badge_icon = &kSupervisorAccountCircleIcon;
        break;
      case BADGE_TYPE_CHILD:
        badge_icon = &kAccountChildCircleIcon;
        break;
      case BADGE_TYPE_NONE:
        NOTREACHED();
        break;
    }

    // Create and add image view for badge icon.
    views::ImageView* badge_view = new views::ImageView();
    badge_view->SetImage(gfx::CreateVectorIcon(*badge_icon, kBadgeIconSize,
                                               gfx::kChromeIconGrey));
    badge_view->SizeToPreferredSize();
    AddChildView(badge_view);
    badge_view->SetPosition(
        gfx::Point(kBadgedProfilePhotoWidth - kBadgeIconSize,
                   kBadgedProfilePhotoHeight - kBadgeIconSize));
  }
  SetPreferredSize(
      gfx::Size(kBadgedProfilePhotoWidth, kBadgedProfilePhotoHeight));
}

const char* BadgedProfilePhoto::GetClassName() const {
  return "BadgedProfilePhoto";
}
