// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/signin_partition_manager.h"
#include "chrome/browser/chromeos/login/signin_partition_manager_factory.h"
#include "chrome/browser/chromeos/login/test/oobe_base_test.h"
#include "chrome/browser/chromeos/login/ui/webui_login_display.h"
#include "chromeos/chromeos_switches.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "media/base/media_switches.h"

namespace chromeos {

class WebviewLoginTest : public OobeBaseTest {
 public:
  WebviewLoginTest() {}
  ~WebviewLoginTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kOobeSkipPostLogin);
    command_line->AppendSwitch(::switches::kUseFakeDeviceForMediaStream);
    OobeBaseTest::SetUpCommandLine(command_line);
  }

 protected:
  void ClickNext() {
    ExecuteJsInSigninFrame("document.getElementById('nextButton').click();");
  }

  void ExpectIdentifierPage() {
    // First page: no back button, no close button, refresh button, #identifier
    // input field.
    JsExpect("!$('gaia-navigation').backVisible");
    JsExpect("!$('gaia-navigation').closeVisible");
    JsExpect("$('gaia-navigation').refreshVisible");
    JsExpect("$('signin-frame').src.indexOf('#identifier') != -1");
  }

  void ExpectPasswordPage() {
    // Second page: back button, close button, no refresh button,
    // #challengepassword input field.
    JsExpect("$('gaia-navigation').backVisible");
    JsExpect("$('gaia-navigation').closeVisible");
    JsExpect("!$('gaia-navigation').refreshVisible");
    JsExpect("$('signin-frame').src.indexOf('#challengepassword') != -1");
  }

  bool WebViewVisited(content::BrowserContext* browser_context,
                      content::StoragePartition* expected_storage_partition,
                      bool* out_found,
                      content::WebContents* guest_contents) {
    content::StoragePartition* guest_storage_partition =
        content::BrowserContext::GetStoragePartition(
            browser_context, guest_contents->GetSiteInstance());
    if (guest_storage_partition == expected_storage_partition)
      *out_found = true;

    // Returns true if found - this will exit the iteration early.
    return *out_found;
  }

  // Returns true if a webview on the login UI's main WebContents is currently
  // displaying a WebContents associated with |storage_partition|.
  bool IsWebviewWithStoragePartitionDisplayed(
      content::StoragePartition* storage_partition) {
    bool is_displayed;

    content::WebContents* web_contents = GetLoginUI()->GetWebContents();
    content::BrowserContext* browser_context =
        web_contents->GetBrowserContext();
    guest_view::GuestViewManager* guest_view_manager =
        guest_view::GuestViewManager::FromBrowserContext(browser_context);
    guest_view_manager->ForEachGuest(
        web_contents,
        base::Bind(&WebviewLoginTest::WebViewVisited, base::Unretained(this),
                   browser_context, storage_partition, &is_displayed));

    return is_displayed;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WebviewLoginTest);
};

// Basic signin with username and password.
IN_PROC_BROWSER_TEST_F(WebviewLoginTest, Basic) {
  WaitForGaiaPageLoad();

  ExpectIdentifierPage();

  SetSignFormField("identifier", OobeBaseTest::kFakeUserEmail);
  ClickNext();
  ExpectPasswordPage();

  content::WindowedNotificationObserver session_start_waiter(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources());

  SetSignFormField("password", OobeBaseTest::kFakeUserPassword);
  ClickNext();

  session_start_waiter.Wait();
}

// Fails: http://crbug.com/512648.
IN_PROC_BROWSER_TEST_F(WebviewLoginTest, DISABLED_BackButton) {
  WaitForGaiaPageLoad();

  // Start with identifer page.
  ExpectIdentifierPage();

  // Move to password page.
  SetSignFormField("identifier", OobeBaseTest::kFakeUserEmail);
  ClickNext();
  ExpectPasswordPage();

  // Click back to identifier page.
  JS().Evaluate("$('gaia-navigation').$.backButton.click();");
  ExpectIdentifierPage();

  // Click next to password page, user id is remembered.
  ClickNext();
  ExpectPasswordPage();

  content::WindowedNotificationObserver session_start_waiter(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources());

  // Finish sign-up.
  SetSignFormField("password", OobeBaseTest::kFakeUserPassword);
  ClickNext();

  session_start_waiter.Wait();
}

IN_PROC_BROWSER_TEST_F(WebviewLoginTest, AllowGuest) {
  WaitForGaiaPageLoad();
  JsExpect("!$('guest-user-header-bar-item').hidden");
  CrosSettings::Get()->SetBoolean(kAccountsPrefAllowGuest, false);
  JsExpect("$('guest-user-header-bar-item').hidden");
}

// Create new account option should be available only if the settings allow it.
IN_PROC_BROWSER_TEST_F(WebviewLoginTest, AllowNewUser) {
  WaitForGaiaPageLoad();

  std::string frame_url = "$('gaia-signin').gaiaAuthHost_.reloadUrl_";
  // New users are allowed.
  JsExpect(frame_url + ".search('flow=nosignup') == -1");

  // Disallow new users - we also need to set a whitelist due to weird logic.
  CrosSettings::Get()->Set(kAccountsPrefUsers, base::ListValue());
  CrosSettings::Get()->SetBoolean(kAccountsPrefAllowNewUser, false);
  WaitForGaiaPageReload();

  // flow=nosignup indicates that user creation is not allowed.
  JsExpect(frame_url + ".search('flow=nosignup') != -1");
}

IN_PROC_BROWSER_TEST_F(WebviewLoginTest, EmailPrefill) {
  WaitForGaiaPageLoad();
  JS().ExecuteAsync("Oobe.showSigninUI('user@example.com')");
  WaitForGaiaPageReload();
  EXPECT_EQ(fake_gaia_->prefilled_email(), "user@example.com");
}

IN_PROC_BROWSER_TEST_F(WebviewLoginTest, StoragePartitionHandling) {
  WaitForGaiaPageLoad();

  // Start with identifer page.
  ExpectIdentifierPage();

  // WebContents of the embedding frame
  content::WebContents* web_contents = GetLoginUI()->GetWebContents();
  content::BrowserContext* browser_context = web_contents->GetBrowserContext();

  std::string signin_frame_partition_name_1;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents->GetMainFrame(),
      "window.domAutomationController.send($('signin-frame').partition);",
      &signin_frame_partition_name_1));
  content::StoragePartition* signin_frame_partition_1 =
      login::GetSigninPartition();

  EXPECT_FALSE(signin_frame_partition_name_1.empty());
  EXPECT_EQ(login::SigninPartitionManagerFactory::GetForBrowserContext(
                browser_context)
                ->GetCurrentStoragePartitionName(),
            signin_frame_partition_name_1);
  EXPECT_TRUE(IsWebviewWithStoragePartitionDisplayed(signin_frame_partition_1));

  // Press the back button at a sign-in screen without pre-existing users to
  // start a new sign-in attempt.
  JS().Evaluate("$('signin-back-button').fire('tap')");
  WaitForGaiaPageReload();
  // Expect that we got back to the identifier page, as there are no known users
  // so the sign-in screen will not display user pods.
  ExpectIdentifierPage();

  std::string signin_frame_partition_name_2;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents->GetMainFrame(),
      "window.domAutomationController.send($('signin-frame').partition);",
      &signin_frame_partition_name_2));
  content::StoragePartition* signin_frame_partition_2 =
      login::GetSigninPartition();

  EXPECT_FALSE(signin_frame_partition_name_2.empty());
  EXPECT_EQ(login::SigninPartitionManagerFactory::GetForBrowserContext(
                browser_context)
                ->GetCurrentStoragePartitionName(),
            signin_frame_partition_name_2);
  EXPECT_TRUE(IsWebviewWithStoragePartitionDisplayed(signin_frame_partition_2));

  // Make sure that the partitions differ and that the old one is not in use
  // anymore.
  EXPECT_NE(signin_frame_partition_name_1, signin_frame_partition_name_2);
  EXPECT_NE(signin_frame_partition_1, signin_frame_partition_2);
  EXPECT_FALSE(
      IsWebviewWithStoragePartitionDisplayed(signin_frame_partition_1));
}

// Tests that requesting webcam access from the login screen works correctly.
// This is needed for taking profile pictures.
IN_PROC_BROWSER_TEST_F(WebviewLoginTest, RequestCamera) {
  WaitForGaiaPageLoad();

  // Video devices should be allowed from the login screen.
  content::WebContents* web_contents = GetLoginUI()->GetWebContents();
  bool getUserMediaSuccess = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents->GetMainFrame(),
      "navigator.getUserMedia("
      "    {video: true},"
      "    function() { window.domAutomationController.send(true); },"
      "    function() { window.domAutomationController.send(false); });",
      &getUserMediaSuccess));
  EXPECT_TRUE(getUserMediaSuccess);

  // Audio devices should be denied from the login screen.
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents->GetMainFrame(),
      "navigator.getUserMedia("
      "    {audio: true},"
      "    function() { window.domAutomationController.send(true); },"
      "    function() { window.domAutomationController.send(false); });",
      &getUserMediaSuccess));
  EXPECT_FALSE(getUserMediaSuccess);
}

}  // namespace chromeos
