// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_SCROLLABLE_USERS_LIST_VIEW_H_
#define ASH_LOGIN_UI_SCROLLABLE_USERS_LIST_VIEW_H_

#include <vector>

#include "ash/ash_export.h"
#include "ash/login/ui/login_display_style.h"
#include "ash/public/interfaces/login_user_info.mojom.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/scroll_view.h"

namespace views {
class View;
class BoxLayout;
}  // namespace views

namespace ash {

class LoginUserView;

// Scrollable list of the users. Stores the list of login user views. Can be
// styled with GradientParams that define gradient tinting at the top and at the
// bottom. Can be styled with LayoutParams that define spacing and sizing.
class ASH_EXPORT ScrollableUsersListView : public views::ScrollView {
 public:
  // TestApi is used for tests to get internal implementation details.
  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(ScrollableUsersListView* view);
    ~TestApi();

    const std::vector<LoginUserView*>& user_views() const;

   private:
    ScrollableUsersListView* const view_;
  };

  struct ASH_EXPORT GradientParams {
    SkColor color_from;
    SkColor color_to;
    SkScalar height;
  };

  struct ASH_EXPORT LayoutParams {
    LoginDisplayStyle display_style;
    int between_child_spacing;
    gfx::Insets insets_landscape;
    gfx::Insets insets_portrait;
  };

  using OnUserViewTap = base::RepeatingCallback<void(int)>;

  ScrollableUsersListView(const std::vector<mojom::LoginUserInfoPtr>& users,
                          const OnUserViewTap& on_user_view_tap_,
                          std::unique_ptr<GradientParams> gradient_params,
                          std::unique_ptr<LayoutParams> layout_params);
  ~ScrollableUsersListView() override;

  // Returns user view at |index| if it exists or nullptr otherwise.
  LoginUserView* GetUserViewAtIndex(int index);

  // views::View:
  void Layout() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;

 private:
  views::BoxLayout* layout_ = nullptr;

  std::vector<LoginUserView*> user_views_;

  std::unique_ptr<GradientParams> gradient_params_;
  std::unique_ptr<LayoutParams> layout_params_;

  DISALLOW_COPY_AND_ASSIGN(ScrollableUsersListView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_SCROLLABLE_USERS_LIST_VIEW_H_