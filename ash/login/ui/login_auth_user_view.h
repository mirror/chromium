// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_AUTH_USER_VIEW_H_
#define ASH_LOGIN_UI_LOGIN_AUTH_USER_VIEW_H_

#include "ash/ash_export.h"
#include "ash/login/ui/login_password_view.h"
#include "ash/public/interfaces/user_info.mojom.h"
#include "base/optional.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace ash {

class LoginUserView;
class LoginPasswordView;
class LoginPinView;

// Wraps a UserView which also has authentication available. Adds additional
// views below the UserView instance which show authentication UIs.
class ASH_EXPORT LoginAuthUserView : public views::View {
 public:
  // TestApi is used for tests to get internal implementation details.
  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(LoginAuthUserView* view);
    ~TestApi();

    LoginPasswordView* password_view() const;

   private:
    LoginAuthUserView* view_;
  };

  // Flags which describe the set of currently visible auth methods.
  enum AuthMethods {
    AUTH_NONE = 0,              // No extra auth methods.
    AUTH_PIN = 1 << 0,          // Display PIN keyboard.
    AUTH_EASY_UNLOCK = 1 << 1,  // Display easy unlock icon.
    AUTH_TAP = 1 << 2,          // Tap to unlock.
  };

  LoginAuthUserView(const mojom::UserInfoPtr& user);
  ~LoginAuthUserView() override;

  // Set the displayed set of auth methods. |auth_methods| contains or-ed
  // together AuthMethod values.
  void SetAuthMethods(uint32_t auth_methods);
  AuthMethods auth_methods() const { return auth_methods_; }

  void UpdateForUser(const mojom::UserInfoPtr& user);

  AccountId active_user() const { return active_user_; }

  // views::View:
  const char* GetClassName() const override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  // Called when the user submits an auth method. Runs mojo call.
  void OnAuthSubmit(bool is_pin, const base::string16& password);

  AccountId active_user_;
  AuthMethods auth_methods_ = AUTH_NONE;
  LoginUserView* user_view_ = nullptr;
  LoginPasswordView* password_view_ = nullptr;
  LoginPinView* pin_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(LoginAuthUserView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_AUTH_USER_VIEW_H_
