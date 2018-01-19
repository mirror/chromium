// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/scrollable_users_list_view.h"

#include "ash/login/ui/layout_util.h"
#include "ash/login/ui/login_display_style.h"
#include "ash/login/ui/login_user_view.h"
#include "ash/login/ui/non_accessible_view.h"
#include "ash/public/cpp/login_constants.h"
#include "ash/public/interfaces/login_user_info.mojom.h"
#include "ash/shell.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_analysis.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

// Vertical padding between each entry in the medium density user row.
constexpr int kMediumDensityVerticalDistanceBetweenUsersDp = 53;

// Horizontal padding left and right of the high density user list.
constexpr int kHighDensityHorizontalPaddingLeftOfUserListLandscapeDp = 72;
constexpr int kHighDensityHorizontalPaddingRightOfUserListLandscapeDp = 72;
constexpr int kHighDensityHorizontalPaddingLeftOfUserListPortraitDp = 46;
constexpr int kHighDensityHorizontalPaddingRightOfUserListPortraitDp = 12;

// Vertical padding of the high density user list.
constexpr int kHighDensityVerticalPaddingOfUserListLandscapeDp = 72;
constexpr int kHighDensityVerticalPaddingOfUserListPortraitDp = 66;

// Vertical padding between each entry in the extra-small user row.
constexpr int kHighDensityVerticalDistanceBetweenUsersDp = 32;

// Height of gradient shown at the top/bottom of high density user list.
constexpr int kHighDensityGradientHeightDp = 112;

ScrollableUsersListView::LayoutParams GetLayoutParams(LoginDisplayStyle style) {
  switch (style) {
    case LoginDisplayStyle::kExtraSmall: {
      auto layout_params = ScrollableUsersListView::LayoutParams();
      layout_params.display_style = LoginDisplayStyle::kExtraSmall;
      layout_params.between_child_spacing =
          kHighDensityVerticalDistanceBetweenUsersDp;
      layout_params.insets_landscape =
          gfx::Insets(kHighDensityVerticalPaddingOfUserListLandscapeDp,
                      kHighDensityHorizontalPaddingLeftOfUserListLandscapeDp,
                      kHighDensityVerticalPaddingOfUserListLandscapeDp,
                      kHighDensityHorizontalPaddingRightOfUserListLandscapeDp);
      layout_params.insets_portrait =
          gfx::Insets(kHighDensityVerticalPaddingOfUserListPortraitDp,
                      kHighDensityHorizontalPaddingLeftOfUserListPortraitDp,
                      kHighDensityVerticalPaddingOfUserListPortraitDp,
                      kHighDensityHorizontalPaddingRightOfUserListPortraitDp);
      return layout_params;
    }
    case LoginDisplayStyle::kSmall: {
      auto layout_params = ScrollableUsersListView::LayoutParams();
      layout_params.between_child_spacing =
          kMediumDensityVerticalDistanceBetweenUsersDp;
      layout_params.display_style = LoginDisplayStyle::kSmall;
      return layout_params;
    }
    default:
      NOTREACHED();
      return ScrollableUsersListView::LayoutParams();
  }
}

ScrollableUsersListView::GradientParams GetGradientParams(
    LoginDisplayStyle style) {
  switch (style) {
    case LoginDisplayStyle::kExtraSmall: {
      SkColor dark_muted_color =
          Shell::Get()->wallpaper_controller()->GetProminentColor(
              color_utils::ColorProfile(color_utils::LumaRange::DARK,
                                        color_utils::SaturationRange::MUTED));
      SkColor tint_color = color_utils::GetResultingPaintColor(
          SkColorSetA(login_constants::kDefaultBaseColor,
                      login_constants::kTranslucentColorDarkenAlpha),
          SkColorSetA(dark_muted_color, SK_AlphaOPAQUE));
      tint_color =
          SkColorSetA(tint_color, login_constants::kScrollTranslucentAlpha);

      auto params = ScrollableUsersListView::GradientParams();
      params.color_from = dark_muted_color;
      params.color_to = tint_color;
      params.height = kHighDensityGradientHeightDp;
      return params;
    }
    case LoginDisplayStyle::kSmall: {
      auto params = ScrollableUsersListView::GradientParams();
      params.height = 0.f;
      return params;
    }
    default:
      NOTREACHED();
      return ScrollableUsersListView::GradientParams();
  }
}

}  // namespace

ScrollableUsersListView::TestApi::TestApi(ScrollableUsersListView* view)
    : view_(view) {}

ScrollableUsersListView::TestApi::~TestApi() = default;

const std::vector<LoginUserView*>&
ScrollableUsersListView::TestApi::user_views() const {
  return view_->user_views_;
}

ScrollableUsersListView::ScrollableUsersListView(
    const std::vector<mojom::LoginUserInfoPtr>& users,
    const OnUserViewTap& on_user_view_tap,
    LoginDisplayStyle display_style)
    : views::ScrollView() {
  layout_params_ = GetLayoutParams(display_style);
  gradient_params_ = GetGradientParams(display_style);

  auto* contents = new NonAccessibleView();
  layout_ = contents->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(),
      layout_params_.between_child_spacing));
  layout_->SetDefaultFlex(1);
  layout_->set_minimum_cross_axis_size(
      LoginUserView::WidthForLayoutStyle(layout_params_.display_style));
  layout_->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::MAIN_AXIS_ALIGNMENT_CENTER);

  for (std::size_t i = 1u; i < users.size(); ++i) {
    auto* view = new LoginUserView(
        layout_params_.display_style, false /*show_dropdown*/,
        base::BindRepeating(on_user_view_tap, i - 1) /*on_tap*/);
    user_views_.push_back(view);
    view->UpdateForUser(users[i], false /*animate*/);
    contents->AddChildView(view);
  }

  SetContents(contents);
  SetBackgroundColor(SK_ColorTRANSPARENT);
}

ScrollableUsersListView::~ScrollableUsersListView() = default;

LoginUserView* ScrollableUsersListView::GetUserViewAtIndex(int index) {
  return static_cast<size_t>(index) < user_views_.size() ? user_views_[index]
                                                         : nullptr;
}

void ScrollableUsersListView::Layout() {
  DCHECK(layout_);
  bool shouldShowLandscape =
      login_layout_util::ShouldShowLandscape(GetWidget());
  layout_->set_inside_border_insets(shouldShowLandscape
                                        ? layout_params_.insets_landscape
                                        : layout_params_.insets_portrait);
  layout_->Layout(contents());

  if (parent()) {
    int contents_height = contents()->size().height();
    int parent_height = parent()->size().height();
    // Adjust height of the content. In extra small style, contents occupies the
    // whole height of the parent. In small style, content is centered
    // vertically.
    ClipHeightTo(layout_params_.display_style == LoginDisplayStyle::kExtraSmall
                     ? parent_height
                     : contents_height,
                 parent_height);
  }
  ScrollView::Layout();
}

void ScrollableUsersListView::OnPaintBackground(gfx::Canvas* canvas) {
  SkScalar view_height = base::checked_cast<SkScalar>(height());
  SkPoint points[2] = {SkPoint(), SkPoint::Make(0.f, view_height)};
  SkScalar position1 = gradient_params_.height / view_height;
  SkScalar position2 = 1.f - position1;
  SkScalar positions[4] = {0.f, position1, position2, 1.f};
  SkColor colors[4] = {gradient_params_.color_from, gradient_params_.color_to,
                       gradient_params_.color_to, gradient_params_.color_from};

  cc::PaintFlags flags;
  flags.setShader(cc::PaintShader::MakeLinearGradient(
      points, colors, positions, 4, SkShader::kClamp_TileMode));
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawRect(GetLocalBounds(), flags);
}

}  // namespace ash
