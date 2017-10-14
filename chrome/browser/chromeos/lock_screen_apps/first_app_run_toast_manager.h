// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_FIRST_APP_RUN_TOAST_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_FIRST_APP_RUN_TOAST_MANAGER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "ui/aura/window_observer.h"
#include "ui/views/widget/widget_observer.h"

class Profile;

namespace views {
class Widget;
}

namespace extensions {
class AppWindow;
}  // namespace extensions

namespace lock_screen_apps {

class FirstAppRunToastManager : public aura::WindowObserver,
                                public views::WidgetObserver {
 public:
  explicit FirstAppRunToastManager(Profile* profile);
  ~FirstAppRunToastManager() override;

  void RunForAppWindow(extensions::AppWindow* app_window);
  void Reset();

  // views::WidgetObserver:
  void OnWidgetDestroyed(views::Widget* widget) override;

  // aura::WindowObserver:
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override;

  views::Widget* widget() { return toast_widget_; }

 private:
  void CreateAndShowToastDialog();
  void ToastDialogDismissed();

  Profile* const profile_;

  extensions::AppWindow* app_window_ = nullptr;
  views::Widget* toast_widget_ = nullptr;

  ScopedObserver<views::Widget, views::WidgetObserver> toast_widget_observer_;
  ScopedObserver<aura::Window, aura::WindowObserver> app_window_observer_;

  base::WeakPtrFactory<FirstAppRunToastManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FirstAppRunToastManager);
};

}  // namespace lock_screen_apps

#endif  // CHROME_BROWSER_CHROMEOS_LOCK_SCREEN_APPS_FIRST_APP_RUN_TOAST_MANAGER_H_
