// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOCK_DEBUG_VIEW_H_
#define ASH_LOGIN_UI_LOCK_DEBUG_VIEW_H_

#include <memory>
#include <vector>

#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace ash {

class LoginDataSource;
class LockContentsView;

// Contains the debug UI strip (ie, add user, remove user buttons)
class LockDebugView : public views::View, public views::ButtonListener {
 public:
  explicit LockDebugView(LoginDataSource* data_source);
  ~LockDebugView() override;

  // views::ButtonListener:
  void Layout() override;
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  class DebugDataSourceTransformer;

  void RebuildUserColumn();

  std::unique_ptr<DebugDataSourceTransformer> const debug_data_source_;
  size_t num_users_ = 1;

  LockContentsView* lock_;

  views::View* debug_;
  views::View* add_user_;
  views::View* remove_user_;

  views::View* user_column_;
  std::vector<views::View*> user_column_entries_toggle_pin_;

  DISALLOW_COPY_AND_ASSIGN(LockDebugView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOCK_DEBUG_VIEW_H_