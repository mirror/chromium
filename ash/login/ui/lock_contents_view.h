// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_CONTENTS_VIEW_H_
#define ASH_LOGIN_UI_CONTENTS_VIEW_H_

#include "ash/ash_export.h"
#include "ash/login/ui/login_data_source.h"
#include "base/macros.h"
#include "ui/views/view.h"

namespace views {
class ScrollView;
}

namespace ash {

class LoginAuthUserView;
class LoginUserView;

// LockContentsView hosts the root view for the lock screen. All other lock
// screen views are embedded within this one. LockContentsView is per-display,
// but it is always shown on the primary display. There is only one instance
// at a time.
class ASH_EXPORT LockContentsView : public views::View,
                                    public LoginDataSource::Observer {
 public:
  // TestApi is used for tests to get internal implementation details.
  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(LockContentsView* view);
    ~TestApi();

    LoginAuthUserView* auth_user_view() const;
    const std::vector<LoginUserView*>& user_views() const;

   private:
    LockContentsView* const view_;
  };

  explicit LockContentsView(LoginDataSource* data_source_);
  ~LockContentsView() override;

  // views::View:
  void Layout() override;

  // LoginDataSource::Observer:
  void OnUsersChanged(
      const std::vector<ash::mojom::UserInfoPtr>& users) override;
  void SetPinEnabledForUser(const AccountId& user, bool enabled) override;

 private:
  friend class TestApi;

  struct UserState {
    UserState(AccountId account_id);

    AccountId account_id;
    bool show_pin = false;
  };

  // Tries to lookup the stored state for |user|. Returns an unowned pointer
  // that is invalidated whenver |users_| changes.
  UserState* FindStateForUser(const AccountId& user);

  void UpdateAuthMethodsForAuthUser();

  std::vector<UserState> users_;

  LoginDataSource* const data_source_;  // Unowned.
  LoginAuthUserView* auth_user_view_;
  // All non-auth users; |auth_user_view_| is not contained in this list.
  std::vector<LoginUserView*> user_views_;
  views::ScrollView* scroller_;
  views::View* background_;

  DISALLOW_COPY_AND_ASSIGN(LockContentsView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_CONTENTS_VIEW_H_
