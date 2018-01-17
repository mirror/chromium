// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_H_

#include <string>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/login/auth/auth_prewarmer.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/ui/login_display.h"
#include "content/public/browser/notification_observer.h"
#include "ui/gfx/native_widget_types.h"

class AccountId;
class ScopedKeepAlive;

namespace wm {
class ScopedDragDropDisabler;
}

namespace chromeos {

class ArcKioskController;
class AppLaunchController;
class DemoAppLauncher;
class LoginScreenContext;
class OobeUI;
class WebUILoginView;
class WizardController;

// An interface that defines OOBE/login screen host.
// Host encapsulates WebUI window OOBE/login controllers,
// UI implementation (such as LoginDisplay).
class LoginDisplayHost : public content::NotificationObserver {
 public:
  // Returns the default LoginDisplayHost instance if it has been created.
  static LoginDisplayHost* default_host() { return default_host_; }

  LoginDisplayHost();
  ~LoginDisplayHost() override;

  // Creates UI implementation specific login display instance (views/WebUI).
  // The caller takes ownership of the returned value.
  virtual LoginDisplay* CreateLoginDisplay(
      LoginDisplay::Delegate* delegate) = 0;

  // Returns corresponding native window.
  virtual gfx::NativeWindow GetNativeWindow() const = 0;

  // Returns instance of the OOBE WebUI.
  virtual OobeUI* GetOobeUI() const = 0;

  // Returns the current login view.
  virtual WebUILoginView* GetWebUILoginView() const = 0;

  // Called when browsing session starts before creating initial browser.
  void BeforeSessionStart();

  // Called when user enters or returns to browsing session so LoginDisplayHost
  // instance may delete itself. |completion_callback| will be invoked when the
  // instance is gone.
  virtual void Finalize(base::OnceClosure completion_callback) = 0;

  // Toggles status area visibility.
  virtual void SetStatusAreaVisible(bool visible) = 0;

  // Starts out-of-box-experience flow or shows other screen handled by
  // Wizard controller i.e. camera, recovery.
  // One could specify start screen with |first_screen|.
  virtual void StartWizard(OobeScreen first_screen) = 0;

  // Returns current WizardController, if it exists.
  // Result should not be stored.
  virtual WizardController* GetWizardController() = 0;

  // Returns current AppLaunchController, if it exists.
  // Result should not be stored.
  AppLaunchController* GetAppLaunchController();

  // Starts screen for adding user into session.
  // |completion_callback| is invoked after login display host shutdown.
  // |completion_callback| can be null.
  virtual void StartUserAdding(base::OnceClosure completion_callback) = 0;

  // Cancel addint user into session.
  virtual void CancelUserAdding() = 0;

  // Starts sign in screen.
  void StartSignInScreen(const LoginScreenContext& context);

  // Invoked when system preferences that affect the signin screen have changed.
  virtual void OnPreferencesChanged() = 0;

  // Initiates authentication network prewarming.
  void PrewarmAuthentication();

  // Starts app launch splash screen. If |is_auto_launch| is true, the app is
  // being auto-launched with no delay.
  void StartAppLaunch(const std::string& app_id,
                      bool diagnostic_mode,
                      bool is_auto_launch);

  // Starts the demo app launch.
  void StartDemoAppLaunch();

  // Starts ARC kiosk splash screen.
  void StartArcKiosk(const AccountId& account_id);

  // Start voice interaction OOBE.
  virtual void StartVoiceInteractionOobe() = 0;

  // Returns whether current host is for voice interaction OOBE.
  virtual bool IsVoiceInteractionOobe() = 0;

  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 protected:
  virtual void OnStartSignInScreen(const LoginScreenContext& context) = 0;
  virtual void OnStartAppLaunch() = 0;
  virtual void OnStartArcKiosk() = 0;
  // Called when the first browser window is created, but before it's shown.
  virtual void OnBrowserCreated() = 0;

  // Deletes |auth_prewarmer_|.
  void OnAuthPrewarmDone();

  // Marks display host for deletion.
  void ShutdownDisplayHost();

  // Active instance of authentication prewarmer.
  std::unique_ptr<AuthPrewarmer> auth_prewarmer_;

  // App launch controller.
  std::unique_ptr<AppLaunchController> app_launch_controller_;

  // Demo app launcher.
  std::unique_ptr<DemoAppLauncher> demo_app_launcher_;

  // ARC kiosk controller.
  std::unique_ptr<ArcKioskController> arc_kiosk_controller_;

  content::NotificationRegistrar registrar_;

 private:
  // Global LoginDisplayHost instance.
  static LoginDisplayHost* default_host_;

  // Has ShutdownDisplayHost() already been called?  Used to avoid posting our
  // own deletion to the message loop twice if the user logs out while we're
  // still in the process of cleaning up after login (http://crbug.com/134463).
  bool shutting_down_ = false;

  // Make sure chrome won't exit while we are at login/oobe screen.
  std::unique_ptr<ScopedKeepAlive> keep_alive_;

  // Keeps a copy of the old Drag'n'Drop client, so that it would be disabled
  // during a login session and restored afterwards.
  std::unique_ptr<wm::ScopedDragDropDisabler> scoped_drag_drop_disabler_;

  // True if session start is in progress.
  bool session_starting_ = false;

  base::WeakPtrFactory<LoginDisplayHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LoginDisplayHost);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_H_
