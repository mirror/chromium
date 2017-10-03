// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_TEST_BASE_H_
#define ASH_LOGIN_UI_LOGIN_TEST_BASE_H_

#include "ash/login/ui/login_data_dispatcher.h"
#include "ash/public/interfaces/user_info.mojom.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"

namespace views {
class View;
class Widget;
}  // namespace views

namespace ash {

// Base test fixture for testing the views-based login and lock screens. This
// class provides easy access to types which the login/lock frequently need.
class LoginTestBase : public AshTestBase {
 public:
  LoginTestBase();
  ~LoginTestBase() override;

  // Creates and displays a widget containing |content|. The created widget
  // will be accessible using |widget()|. May be called at most once.
  void ShowWidgetWithContent(views::View* content);

  // Creates a widget containing |content|. Unlike |ShowWidgetWithContent|, the
  // ownership of the created widget is passed onto caller.
  // NOTE: If your test needs only a single widget, prefer
  // |ShowWidgetWithContent|.
  views::Widget* CreateWidgetWithContent(views::View* content);

  views::Widget* widget() const { return widget_; }

  // Utility method to create a new |mojom::UserInfoPtr| instance.
  mojom::UserInfoPtr CreateUser(const std::string& name) const;

  // Changes the active number of users. Fires an event on |data_dispatcher()|.
  void SetUserCount(size_t count);

  const std::vector<mojom::UserInfoPtr>& users() const { return users_; }

  LoginDataDispatcher* data_dispatcher() { return &data_dispatcher_; }

  // AshTestBase:
  void SetUp() override;
  void TearDown() override;

 private:
  class WidgetDelegate;

  // The widget created using |ShowWidgetWithContent|.
  views::Widget* widget_ = nullptr;

  std::vector<mojom::UserInfoPtr> users_;

  LoginDataDispatcher data_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(LoginTestBase);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_TEST_BASE_H_
