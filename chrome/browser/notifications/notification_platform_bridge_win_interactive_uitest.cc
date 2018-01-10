// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <windows.data.xml.dom.h>
#include <wrl/client.h>

#include "base/run_loop.h"
#include "base/win/scoped_hstring.h"
#include "base/win/windows_version.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/mock_itoastnotification.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/browser/notifications/notification_platform_bridge_win.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"

namespace mswr = Microsoft::WRL;
namespace winui = ABI::Windows::UI;
namespace winxml = ABI::Windows::Data::Xml;

class NotificationPlatformBridgeWinUITest : public InProcessBrowserTest {
 public:
  NotificationPlatformBridgeWinUITest() = default;
  ~NotificationPlatformBridgeWinUITest() override = default;

  void SetUpOnMainThread() override {
    display_service_tester_ =
        std::make_unique<NotificationDisplayServiceTester>(
            browser()->profile());
  }

  void TearDownOnMainThread() override { display_service_tester_.reset(); }

  void HandleOperation(const base::RepeatingClosure& quit_task,
                       NotificationCommon::Operation operation,
                       NotificationHandler::Type notification_type,
                       const GURL& origin,
                       const std::string& notification_id,
                       const base::Optional<int>& action_index,
                       const base::Optional<base::string16>& reply,
                       const base::Optional<bool>& by_user) {
    last_operation_ = operation;
    last_notification_type_ = notification_type;
    last_origin_ = origin;
    last_notification_id_ = notification_id;
    last_action_index_ = action_index;
    last_reply_ = reply;
    last_by_user_ = by_user;
    quit_task.Run();
  }

 protected:
  bool ValidateNotificationValues(NotificationCommon::Operation operation,
                                  NotificationHandler::Type notification_type,
                                  const GURL& origin,
                                  const std::string& notification_id,
                                  const base::Optional<int>& action_index,
                                  const base::Optional<base::string16>& reply,
                                  const base::Optional<bool>& by_user) {
    return operation == last_operation_ &&
           notification_type == last_notification_type_ &&
           origin == last_origin_ && notification_id == last_notification_id_ &&
           action_index == last_action_index_ && reply == last_reply_ &&
           by_user == last_by_user_;
  }

  std::unique_ptr<NotificationDisplayServiceTester> display_service_tester_;

  NotificationCommon::Operation last_operation_;
  NotificationHandler::Type last_notification_type_;
  GURL last_origin_;
  std::string last_notification_id_;
  base::Optional<int> last_action_index_;
  base::Optional<base::string16> last_reply_;
  base::Optional<bool> last_by_user_;

  bool delegate_called_ = false;
};

class MockIToastActivatedEventArgs
    : public winui::Notifications::IToastActivatedEventArgs {
 public:
  explicit MockIToastActivatedEventArgs(const base::string16& args)
      : arguments_(args) {}
  virtual ~MockIToastActivatedEventArgs() = default;

  // TODO(finnur): Would also like to remove these 6 functions...
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                           void** ppvObject) override {
    return E_NOTIMPL;
  }
  ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
  ULONG STDMETHODCALLTYPE Release() override { return 0; }
  HRESULT STDMETHODCALLTYPE GetIids(ULONG* iidCount, IID** iids) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING* className) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel* trustLevel) override {
    return E_NOTIMPL;
  }

  HRESULT STDMETHODCALLTYPE get_Arguments(HSTRING* value) override {
    base::win::ScopedHString arguments =
        base::win::ScopedHString::Create(arguments_);
    *value = arguments.get();
    return S_OK;
  }

 private:
  base::string16 arguments_;

  DISALLOW_COPY_AND_ASSIGN(MockIToastActivatedEventArgs);
};

IN_PROC_BROWSER_TEST_F(NotificationPlatformBridgeWinUITest, HandleEvent) {
  // This test exercises a feature that is not enabled in older versions of
  // Windows.
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;

  const wchar_t kXmlDoc[] =
      LR"(<toast launch="0|Default|0|https://example.com/|notification_id">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
 <actions>
 <action content="Click" arguments="args" activationType="foreground"/>
 </actions>
</toast>
)";

  MockIToastNotification toast(kXmlDoc);
  MockIToastActivatedEventArgs args(L"buttonIndex=1");

  base::RunLoop run_loop;
  display_service_tester_->SetProcessNotificationOperationDelegate(
      base::BindRepeating(&NotificationPlatformBridgeWinUITest::HandleOperation,
                          base::Unretained(this), run_loop.QuitClosure()));

  // Simulate clicks on the toast.
  NotificationPlatformBridgeWin* bridge =
      reinterpret_cast<NotificationPlatformBridgeWin*>(
          g_browser_process->notification_platform_bridge());
  bridge->ForwardHandleEventForTesting(NotificationCommon::CLICK, &toast, &args,
                                       base::nullopt);
  run_loop.Run();

  // Validate the click values.
  base::Optional<int> action_index = 1;
  EXPECT_EQ(NotificationCommon::CLICK, last_operation_);
  EXPECT_EQ(NotificationHandler::Type::WEB_PERSISTENT, last_notification_type_);
  EXPECT_EQ(GURL("https://example.com/"), last_origin_);
  EXPECT_EQ("notification_id", last_notification_id_);
  EXPECT_EQ(action_index, last_action_index_);
  EXPECT_EQ(base::nullopt, last_reply_);
  EXPECT_EQ(base::nullopt, last_by_user_);
}
