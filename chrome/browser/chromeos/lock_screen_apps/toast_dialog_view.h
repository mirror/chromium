// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_TOAST_DIALOG_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_TOAST_DIALOG_VIEW_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace views {
class Label;
class Widget;
}  // namespace views

namespace lock_screen_apps {

class ToastDialogView : public views::BubbleDialogDelegateView {
 public:
  ToastDialogView(const base::string16& app_name,
                  base::OnceClosure dismissed_callback);
  ~ToastDialogView() override;

  void Show();

  // views::WidgetDelegate:
  void OnWorkAreaChanged() override;
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;

  // views::BubbleDialogDelegate:
  bool Close() override;
  void AddedToWidget() override;
  int GetDialogButtons() const override;
  bool ShouldShowCloseButton() const override;

 private:
  void AdjustDialogBounds(views::Widget* widget);

  base::string16 app_name_;
  base::OnceClosure dismissed_callback_;

  views::Label* label_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ToastDialogView);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_TOAST_DIALOG_VIEW_H_
