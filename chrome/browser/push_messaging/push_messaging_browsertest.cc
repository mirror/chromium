// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browsing_data/browsing_data_helper.h"
#include "chrome/browser/browsing_data/browsing_data_remover.h"
#include "chrome/browser/browsing_data/browsing_data_remover_test_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/notifications/notification_test_util.h"
#include "chrome/browser/notifications/platform_notification_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/push_messaging/push_messaging_application_id.h"
#include "chrome/browser/push_messaging/push_messaging_constants.h"
#include "chrome/browser/push_messaging/push_messaging_service_factory.h"
#include "chrome/browser/push_messaging/push_messaging_service_impl.h"
#include "chrome/browser/services/gcm/fake_gcm_profile_service.h"
#include "chrome/browser/services/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "ui/base/window_open_disposition.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

namespace {
// Responds to a confirm infobar by accepting or cancelling it. Responds to at
// most one infobar.
class InfoBarResponder : public infobars::InfoBarManager::Observer {
 public:
  InfoBarResponder(Browser* browser, bool accept)
      : infobar_service_(InfoBarService::FromWebContents(
            browser->tab_strip_model()->GetActiveWebContents())),
        accept_(accept),
        has_observed_(false) {
    infobar_service_->AddObserver(this);
  }

  ~InfoBarResponder() override { infobar_service_->RemoveObserver(this); }

  // infobars::InfoBarManager::Observer
  void OnInfoBarAdded(infobars::InfoBar* infobar) override {
    if (has_observed_)
      return;
    has_observed_ = true;
    ConfirmInfoBarDelegate* delegate =
        infobar->delegate()->AsConfirmInfoBarDelegate();
    DCHECK(delegate);

    // Respond to the infobar asynchronously, like a person.
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(
            &InfoBarResponder::Respond, base::Unretained(this), delegate));
  }

 private:
  void Respond(ConfirmInfoBarDelegate* delegate) {
    if (accept_) {
      delegate->Accept();
    } else {
      delegate->Cancel();
    }
  }

  InfoBarService* infobar_service_;
  bool accept_;
  bool has_observed_;
};

// Class to instantiate on the stack that is meant to be used with
// FakeGCMProfileService. The ::Run() method follows the signature of
// FakeGCMProfileService::UnregisterCallback.
class UnregistrationCallback {
 public:
  UnregistrationCallback()
      : message_loop_runner_(new content::MessageLoopRunner) {}

  void Run(const std::string& app_id) {
    app_id_ = app_id;
    message_loop_runner_->Quit();
  }

  void WaitUntilSatisfied() {
    message_loop_runner_->Run();
  }

  const std::string& app_id() {
    return app_id_;
  }

 private:
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;
  std::string app_id_;
};

}  // namespace

class PushMessagingBrowserTest : public InProcessBrowserTest {
 public:
  PushMessagingBrowserTest() : gcm_service_(nullptr) {}
  ~PushMessagingBrowserTest() override {}

  // InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnablePushMessagePayload);
    command_line->AppendSwitch(switches::kEnablePushMessagingHasPermission);

    InProcessBrowserTest::SetUpCommandLine(command_line);
  }

  // InProcessBrowserTest:
  void SetUp() override {
    https_server_.reset(new net::SpawnedTestServer(
        net::SpawnedTestServer::TYPE_HTTPS,
        net::BaseTestServer::SSLOptions(
            net::BaseTestServer::SSLOptions::CERT_OK),
        base::FilePath(FILE_PATH_LITERAL("chrome/test/data/"))));
    ASSERT_TRUE(https_server_->Start());

#if defined(ENABLE_NOTIFICATIONS)
    notification_manager_.reset(new StubNotificationUIManager);
    notification_service()->SetNotificationUIManagerForTesting(
        notification_manager());
#endif

    InProcessBrowserTest::SetUp();
  }

  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    gcm_service_ = static_cast<gcm::FakeGCMProfileService*>(
        gcm::GCMProfileServiceFactory::GetInstance()->SetTestingFactoryAndUse(
            GetBrowser()->profile(), &gcm::FakeGCMProfileService::Build));
    gcm_service_->set_collect(true);
    push_service_ =
        PushMessagingServiceFactory::GetForProfile(GetBrowser()->profile());

    LoadTestPage();

    InProcessBrowserTest::SetUpOnMainThread();
  }

  // InProcessBrowserTest:
  void TearDown() override {
#if defined(ENABLE_NOTIFICATIONS)
    notification_service()->SetNotificationUIManagerForTesting(nullptr);
#endif

    InProcessBrowserTest::TearDown();
  }

  void LoadTestPage(const std::string& path) {
    ui_test_utils::NavigateToURL(GetBrowser(), https_server_->GetURL(path));
  }

  void LoadTestPage() {
    LoadTestPage(GetTestURL());
  }

  bool RunScript(const std::string& script, std::string* result) {
    return RunScript(script, result, nullptr);
  }

  bool RunScript(const std::string& script, std::string* result,
                 content::WebContents* web_contents) {
    if (!web_contents)
      web_contents = GetBrowser()->tab_strip_model()->GetActiveWebContents();
    return content::ExecuteScriptAndExtractString(web_contents->GetMainFrame(),
                                                  script,
                                                  result);
  }

  void TryToSubscribeSuccessfully(
      const std::string& expected_push_subscription_id);

  PushMessagingApplicationId GetServiceWorkerAppId(
      int64 service_worker_registration_id);

  void SendMessageAndWaitUntilHandled(
      const PushMessagingApplicationId& app_id,
      const gcm::GCMClient::IncomingMessage& message);

  net::SpawnedTestServer* https_server() const { return https_server_.get(); }

  gcm::FakeGCMProfileService* gcm_service() const { return gcm_service_; }

#if defined(ENABLE_NOTIFICATIONS)
  StubNotificationUIManager* notification_manager() const {
    return notification_manager_.get();
  }

  PlatformNotificationServiceImpl* notification_service() const {
    return PlatformNotificationServiceImpl::GetInstance();
  }
#endif

  PushMessagingServiceImpl* push_service() const { return push_service_; }

 protected:
  virtual std::string GetTestURL() {
    return "files/push_messaging/test.html";
  }

  virtual Browser* GetBrowser() const { return browser(); }

 private:
  scoped_ptr<net::SpawnedTestServer> https_server_;
  gcm::FakeGCMProfileService* gcm_service_;
  PushMessagingServiceImpl* push_service_;
  scoped_ptr<StubNotificationUIManager> notification_manager_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingBrowserTest);
};

class PushMessagingBadManifestBrowserTest : public PushMessagingBrowserTest {
  std::string GetTestURL() override {
    return "files/push_messaging/test_bad_manifest.html";
  }
};

IN_PROC_BROWSER_TEST_F(PushMessagingBadManifestBrowserTest,
                       SubscribeFailsNotVisibleMessages) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);
  ASSERT_TRUE(RunScript("subscribePush()", &script_result));
  EXPECT_EQ("AbortError - Registration failed - permission denied",
            script_result);
}

void PushMessagingBrowserTest::TryToSubscribeSuccessfully(
    const std::string& expected_push_subscription_id) {
  std::string script_result;

  EXPECT_TRUE(RunScript("registerServiceWorker()", &script_result));
  EXPECT_EQ("ok - service worker registered", script_result);

  InfoBarResponder accepting_responder(GetBrowser(), true);
  EXPECT_TRUE(RunScript("requestNotificationPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  EXPECT_TRUE(RunScript("subscribePush()", &script_result));
  EXPECT_EQ(std::string(kPushMessagingEndpoint) + " - "
            + expected_push_subscription_id, script_result);
}

PushMessagingApplicationId PushMessagingBrowserTest::GetServiceWorkerAppId(
    int64 service_worker_registration_id) {
  GURL origin = https_server()->GetURL(std::string()).GetOrigin();
  PushMessagingApplicationId application_id = PushMessagingApplicationId::Get(
      GetBrowser()->profile(), origin, service_worker_registration_id);
  EXPECT_TRUE(application_id.IsValid());
  return application_id;
}

void PushMessagingBrowserTest::SendMessageAndWaitUntilHandled(
    const PushMessagingApplicationId& app_id,
    const gcm::GCMClient::IncomingMessage& message) {
  base::RunLoop run_loop;
  push_service()->SetMessageCallbackForTesting(run_loop.QuitClosure());
  push_service()->OnMessage(app_id.app_id_guid(), message);
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       SubscribeSuccessNotificationsGranted) {
  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  PushMessagingApplicationId app_id = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id.app_id_guid(), gcm_service()->last_registered_app_id());
  EXPECT_EQ("1234567890", gcm_service()->last_registered_sender_ids()[0]);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       SubscribeSuccessNotificationsPrompt) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  InfoBarResponder accepting_responder(GetBrowser(), true);
  ASSERT_TRUE(RunScript("subscribePush()", &script_result));
  EXPECT_EQ(std::string(kPushMessagingEndpoint) + " - 1-0", script_result);

  PushMessagingApplicationId app_id = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id.app_id_guid(), gcm_service()->last_registered_app_id());
  EXPECT_EQ("1234567890", gcm_service()->last_registered_sender_ids()[0]);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       SubscribeFailureNotificationsBlocked) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  InfoBarResponder cancelling_responder(GetBrowser(), false);
  ASSERT_TRUE(RunScript("requestNotificationPermission();", &script_result));
  ASSERT_EQ("permission status - denied", script_result);

  ASSERT_TRUE(RunScript("subscribePush()", &script_result));
  EXPECT_EQ("AbortError - Registration failed - permission denied",
            script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, SubscribeFailureNoManifest) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  InfoBarResponder accepting_responder(GetBrowser(), true);
  ASSERT_TRUE(RunScript("requestNotificationPermission();", &script_result));
  ASSERT_EQ("permission status - granted", script_result);

  ASSERT_TRUE(RunScript("removeManifest()", &script_result));
  ASSERT_EQ("manifest removed", script_result);

  ASSERT_TRUE(RunScript("subscribePush()", &script_result));
  EXPECT_EQ("AbortError - Registration failed - no sender id provided",
            script_result);
}

// TODO(johnme): Test subscribing from a worker - see https://crbug.com/437298.

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, SubscribePersisted) {
  std::string script_result;

  // First, test that Service Worker registration IDs are assigned in order of
  // registering the Service Workers, and the (fake) push subscription ids are
  // assigned in order of push subscription (even when these orders are
  // different).

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);
  PushMessagingApplicationId app_id_sw0 = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id_sw0.app_id_guid(), gcm_service()->last_registered_app_id());

  LoadTestPage("files/push_messaging/subscope1/test.html");
  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  LoadTestPage("files/push_messaging/subscope2/test.html");
  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  // Note that we need to reload the page after registering, otherwise
  // navigator.serviceWorker.ready is going to be resolved with the parent
  // Service Worker which still controls the page.
  LoadTestPage("files/push_messaging/subscope2/test.html");
  TryToSubscribeSuccessfully("1-1" /* expected_push_subscription_id */);
  PushMessagingApplicationId app_id_sw2 = GetServiceWorkerAppId(2LL);
  EXPECT_EQ(app_id_sw2.app_id_guid(), gcm_service()->last_registered_app_id());

  LoadTestPage("files/push_messaging/subscope1/test.html");
  TryToSubscribeSuccessfully("1-2" /* expected_push_subscription_id */);
  PushMessagingApplicationId app_id_sw1 = GetServiceWorkerAppId(1LL);
  EXPECT_EQ(app_id_sw1.app_id_guid(), gcm_service()->last_registered_app_id());

  // Now test that the Service Worker registration IDs and push subscription IDs
  // generated above were persisted to SW storage, by checking that they are
  // unchanged despite requesting them in a different order.
  // TODO(johnme): Ideally we would restart the browser at this point to check
  // they were persisted to disk, but that's not currently possible since the
  // test server uses random port numbers for each test (even PRE_Foo and Foo),
  // so we wouldn't be able to load the test pages with the same origin.

  LoadTestPage("files/push_messaging/subscope1/test.html");
  TryToSubscribeSuccessfully("1-2" /* expected_push_subscription_id */);
  EXPECT_EQ(app_id_sw1.app_id_guid(), gcm_service()->last_registered_app_id());

  LoadTestPage("files/push_messaging/subscope2/test.html");
  TryToSubscribeSuccessfully("1-1" /* expected_push_subscription_id */);
  EXPECT_EQ(app_id_sw1.app_id_guid(), gcm_service()->last_registered_app_id());

  LoadTestPage();
  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);
  EXPECT_EQ(app_id_sw1.app_id_guid(), gcm_service()->last_registered_app_id());
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, PushEventSuccess) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  PushMessagingApplicationId app_id = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id.app_id_guid(), gcm_service()->last_registered_app_id());
  EXPECT_EQ("1234567890", gcm_service()->last_registered_sender_ids()[0]);

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("false - is not controlled", script_result);

  LoadTestPage();  // Reload to become controlled.

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("true - is controlled", script_result);

  gcm::GCMClient::IncomingMessage message;
  message.sender_id = "1234567890";
  message.data["data"] = "testdata";
  push_service()->OnMessage(app_id.app_id_guid(), message);
  ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result));
  EXPECT_EQ("testdata", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, PushEventNoServiceWorker) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  PushMessagingApplicationId app_id = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id.app_id_guid(), gcm_service()->last_registered_app_id());
  EXPECT_EQ("1234567890", gcm_service()->last_registered_sender_ids()[0]);

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("false - is not controlled", script_result);

  LoadTestPage();  // Reload to become controlled.

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("true - is controlled", script_result);

  // Unregister service worker. Sending a message should now fail.
  ASSERT_TRUE(RunScript("unregisterServiceWorker()", &script_result));
  ASSERT_EQ("service worker unregistration status: true", script_result);

  // When the push service will receive it next message, given that there is no
  // SW available, it should unregister |app_id|.
  UnregistrationCallback callback;
  gcm_service()->SetUnregisterCallback(base::Bind(&UnregistrationCallback::Run,
                                                  base::Unretained(&callback)));

  gcm::GCMClient::IncomingMessage message;
  message.sender_id = "1234567890";
  message.data["data"] = "testdata";
  push_service()->OnMessage(app_id.app_id_guid(), message);

  callback.WaitUntilSatisfied();
  EXPECT_EQ(app_id.app_id_guid(), callback.app_id());

  // No push data should have been received.
  ASSERT_TRUE(RunScript("resultQueue.popImmediately()", &script_result));
  EXPECT_EQ("null", script_result);
}

#if defined(ENABLE_NOTIFICATIONS)
IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       PushEventEnforcesUserVisibleNotification) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  PushMessagingApplicationId app_id = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id.app_id_guid(), gcm_service()->last_registered_app_id());
  EXPECT_EQ("1234567890", gcm_service()->last_registered_sender_ids()[0]);

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("false - is not controlled", script_result);

  LoadTestPage();  // Reload to become controlled.

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("true - is controlled", script_result);

  notification_manager()->CancelAll();
  ASSERT_EQ(0u, notification_manager()->GetNotificationCount());

  // We'll need to specify the web_contents in which to eval script, since we're
  // going to run script in a background tab.
  content::WebContents* web_contents =
      GetBrowser()->tab_strip_model()->GetActiveWebContents();

  // If the site is visible in an active tab, we should not force a notification
  // to be shown. Try it twice, since we allow one mistake per 10 push events.
  gcm::GCMClient::IncomingMessage message;
  message.sender_id = "1234567890";
  for (int n = 0; n < 2; n++) {
    message.data["data"] = "testdata";
    SendMessageAndWaitUntilHandled(app_id, message);
    ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result));
    EXPECT_EQ("testdata", script_result);
    EXPECT_EQ(0u, notification_manager()->GetNotificationCount());
  }

  // Open a blank foreground tab so site is no longer visible.
  ui_test_utils::NavigateToURLWithDisposition(
      GetBrowser(), GURL("about:blank"), NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB);

  // If the Service Worker push event handler does not show a notification, we
  // should show a forced one, but only on the 2nd occurrence since we allow one
  // mistake per 10 push events.
  message.data["data"] = "testdata";
  SendMessageAndWaitUntilHandled(app_id, message);
  ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result, web_contents));
  EXPECT_EQ("testdata", script_result);
  EXPECT_EQ(0u, notification_manager()->GetNotificationCount());
  message.data["data"] = "testdata";
  SendMessageAndWaitUntilHandled(app_id, message);
  ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result, web_contents));
  EXPECT_EQ("testdata", script_result);

  EXPECT_EQ(1u, notification_manager()->GetNotificationCount());
  const Notification& forced_notification =
      notification_manager()->GetNotificationAt(0);

  EXPECT_EQ(kPushMessagingForcedNotificationTag, forced_notification.tag());
  EXPECT_TRUE(forced_notification.silent());

  // Currently, this notification will stick around until the user or webapp
  // explicitly dismisses it (though we may change this later).
  message.data["data"] = "shownotification";
  SendMessageAndWaitUntilHandled(app_id, message);
  ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result, web_contents));
  EXPECT_EQ("shownotification", script_result);
  EXPECT_EQ(2u, notification_manager()->GetNotificationCount());

  notification_manager()->CancelAll();
  EXPECT_EQ(0u, notification_manager()->GetNotificationCount());

  // However if the Service Worker push event handler shows a notification, we
  // should not show a forced one.
  message.data["data"] = "shownotification";
  for (int n = 0; n < 9; n++) {
    SendMessageAndWaitUntilHandled(app_id, message);
    ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result, web_contents));
    EXPECT_EQ("shownotification", script_result);
    EXPECT_EQ(1u, notification_manager()->GetNotificationCount());
    EXPECT_EQ("push_test_tag",
              notification_manager()->GetNotificationAt(0).tag());
    notification_manager()->CancelAll();
  }

  // Now that 10 push messages in a row have shown notifications, we should
  // allow the next one to mistakenly not show a notification.
  message.data["data"] = "testdata";
  SendMessageAndWaitUntilHandled(app_id, message);
  ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result, web_contents));
  EXPECT_EQ("testdata", script_result);
  EXPECT_EQ(0u, notification_manager()->GetNotificationCount());
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       PushEventNotificationWithoutEventWaitUntil) {
  std::string script_result;
  content::WebContents* web_contents =
      GetBrowser()->tab_strip_model()->GetActiveWebContents();

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  PushMessagingApplicationId app_id = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id.app_id_guid(), gcm_service()->last_registered_app_id());
  EXPECT_EQ("1234567890", gcm_service()->last_registered_sender_ids()[0]);

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("false - is not controlled", script_result);

  LoadTestPage();  // Reload to become controlled.

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("true - is controlled", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  notification_manager()->SetNotificationAddedCallback(
      message_loop_runner->QuitClosure());

  gcm::GCMClient::IncomingMessage message;
  message.sender_id = "1234567890";
  message.data["data"] = "shownotification-without-waituntil";
  push_service()->OnMessage(app_id.app_id_guid(), message);
  ASSERT_TRUE(RunScript("resultQueue.pop()", &script_result, web_contents));
  EXPECT_EQ("immediate:shownotification-without-waituntil", script_result);

  message_loop_runner->Run();

  ASSERT_EQ(1u, notification_manager()->GetNotificationCount());
  EXPECT_EQ("push_test_tag",
            notification_manager()->GetNotificationAt(0).tag());

  // Verify that the renderer process hasn't crashed.
  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);
}
#endif

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, HasPermissionSaysDefault) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  ASSERT_EQ("permission status - default", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, HasPermissionSaysGranted) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  InfoBarResponder accepting_responder(GetBrowser(), true);
  ASSERT_TRUE(RunScript("requestNotificationPermission();", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  ASSERT_TRUE(RunScript("subscribePush()", &script_result));
  EXPECT_EQ(std::string(kPushMessagingEndpoint) + " - 1-0", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, HasPermissionSaysDenied) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  InfoBarResponder cancelling_responder(GetBrowser(), false);
  ASSERT_TRUE(RunScript("requestNotificationPermission();", &script_result));
  EXPECT_EQ("permission status - denied", script_result);

  ASSERT_TRUE(RunScript("subscribePush()", &script_result));
  EXPECT_EQ("AbortError - Registration failed - permission denied",
            script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - denied", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest, UnsubscribeSuccess) {
  std::string script_result;

  EXPECT_TRUE(RunScript("registerServiceWorker()", &script_result));
  EXPECT_EQ("ok - service worker registered", script_result);

  // Resolves true if there was a subscription.
  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);
  gcm_service()->AddExpectedUnregisterResponse(gcm::GCMClient::SUCCESS);
  ASSERT_TRUE(RunScript("unsubscribePush()", &script_result));
  EXPECT_EQ("unsubscribe result: true", script_result);

  // Resolves false if there was no longer a subscription.
  ASSERT_TRUE(RunScript("unsubscribePush()", &script_result));
  EXPECT_EQ("unsubscribe result: false", script_result);

  // Doesn't reject if there was a network error (deactivates subscription
  // locally anyway).
  TryToSubscribeSuccessfully("1-1" /* expected_push_subscription_id */);
  gcm_service()->AddExpectedUnregisterResponse(gcm::GCMClient::NETWORK_ERROR);
  ASSERT_TRUE(RunScript("unsubscribePush()", &script_result));
  EXPECT_EQ("unsubscribe result: true", script_result);
  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("false - not subscribed", script_result);

  // Doesn't reject if there were other push service errors (deactivates
  // subscription locally anyway).
  TryToSubscribeSuccessfully("1-2" /* expected_push_subscription_id */);
  gcm_service()->AddExpectedUnregisterResponse(
      gcm::GCMClient::INVALID_PARAMETER);
  ASSERT_TRUE(RunScript("unsubscribePush()", &script_result));
  EXPECT_EQ("unsubscribe result: true", script_result);

  // Unsubscribing (with an existing reference to a PushSubscription), after
  // unregistering the Service Worker, just means push subscription isn't found.
  TryToSubscribeSuccessfully("1-3" /* expected_push_subscription_id */);
  ASSERT_TRUE(RunScript("unregisterServiceWorker()", &script_result));
  ASSERT_EQ("service worker unregistration status: true", script_result);
  ASSERT_TRUE(RunScript("unsubscribePush()", &script_result));
  EXPECT_EQ("unsubscribe result: false", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       GlobalResetPushPermissionUnsubscribes) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      message_loop_runner->QuitClosure());

  GetBrowser()->profile()->GetHostContentSettingsMap()->
      ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_PUSH_MESSAGING);

  message_loop_runner->Run();

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - default", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("false - not subscribed", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       LocalResetPushPermissionUnsubscribes) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      message_loop_runner->QuitClosure());

  GURL origin = https_server()->GetURL(std::string()).GetOrigin();
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::FromURLNoWildcard(origin),
      CONTENT_SETTINGS_TYPE_PUSH_MESSAGING,
      std::string(),
      CONTENT_SETTING_DEFAULT);

  message_loop_runner->Run();

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - default", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("false - not subscribed", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       DenyPushPermissionUnsubscribes) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      message_loop_runner->QuitClosure());

  GURL origin = https_server()->GetURL(std::string()).GetOrigin();
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::FromURLNoWildcard(origin),
      CONTENT_SETTINGS_TYPE_PUSH_MESSAGING,
      std::string(),
      CONTENT_SETTING_BLOCK);

  message_loop_runner->Run();

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - denied", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("false - not subscribed", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       GlobalResetNotificationsPermissionUnsubscribes) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      message_loop_runner->QuitClosure());

  GetBrowser()->profile()->GetHostContentSettingsMap()->
      ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_NOTIFICATIONS);

  message_loop_runner->Run();

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - default", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("false - not subscribed", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       LocalResetNotificationsPermissionUnsubscribes) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      message_loop_runner->QuitClosure());

  GURL origin = https_server()->GetURL(std::string()).GetOrigin();
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      std::string(),
      CONTENT_SETTING_DEFAULT);

  message_loop_runner->Run();

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - default", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("false - not subscribed", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       DenyNotificationsPermissionUnsubscribes) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      message_loop_runner->QuitClosure());

  GURL origin = https_server()->GetURL(std::string()).GetOrigin();
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      std::string(),
      CONTENT_SETTING_BLOCK);

  message_loop_runner->Run();

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - denied", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("false - not subscribed", script_result);
}

IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       GrantAlreadyGrantedPermissionDoesNotUnsubscribe) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      base::BarrierClosure(2, message_loop_runner->QuitClosure()));

  GURL origin = https_server()->GetURL(std::string()).GetOrigin();
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      std::string(),
      CONTENT_SETTING_ALLOW);
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::FromURLNoWildcard(origin),
      CONTENT_SETTINGS_TYPE_PUSH_MESSAGING,
      std::string(),
      CONTENT_SETTING_ALLOW);

  message_loop_runner->Run();

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);
}

// This test is testing some non-trivial content settings rules and make sure
// that they are respected with regards to automatic unsubscription. In other
// words, it checks that the push service does not end up unsubscribing origins
// that have push permission with some non-common rules.
IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       AutomaticUnsubscriptionFollowsContentSettingRules) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  scoped_refptr<content::MessageLoopRunner> message_loop_runner =
      new content::MessageLoopRunner;
  push_service()->SetContentSettingChangedCallbackForTesting(
      base::BarrierClosure(4, message_loop_runner->QuitClosure()));

  GURL origin = https_server()->GetURL(std::string()).GetOrigin();
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      std::string(),
      CONTENT_SETTING_ALLOW);
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromString("https://*"),
      ContentSettingsPattern::FromString("https://*"),
      CONTENT_SETTINGS_TYPE_PUSH_MESSAGING,
      std::string(),
      CONTENT_SETTING_ALLOW);
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      std::string(),
      CONTENT_SETTING_DEFAULT);
  GetBrowser()->profile()->GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(origin),
      ContentSettingsPattern::FromURLNoWildcard(origin),
      CONTENT_SETTINGS_TYPE_PUSH_MESSAGING,
      std::string(),
      CONTENT_SETTING_DEFAULT);

  message_loop_runner->Run();

  // The two first rules should give |origin| the permission to use Push even
  // if the rules it used to have have been reset.
  // The Push service should not unsubcribe |origin| because at no point it was
  // left without permission to use Push.

  ASSERT_TRUE(RunScript("hasPermission()", &script_result));
  EXPECT_EQ("permission status - granted", script_result);

  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  EXPECT_EQ("true - subscribed", script_result);
}

// Checks that automatically unsubscribing due to a revoked permission is
// handled well if the sender ID needed to unsubscribe was already deleted.
IN_PROC_BROWSER_TEST_F(PushMessagingBrowserTest,
                       ResetPushPermissionAfterClearingSiteData) {
  std::string script_result;

  TryToSubscribeSuccessfully("1-0" /* expected_push_subscription_id */);

  PushMessagingApplicationId app_id = GetServiceWorkerAppId(0LL);
  EXPECT_EQ(app_id.app_id_guid(), gcm_service()->last_registered_app_id());
  PushMessagingApplicationId stored_app_id = PushMessagingApplicationId::Get(
      GetBrowser()->profile(), app_id.app_id_guid());
  EXPECT_TRUE(stored_app_id.IsValid());

  // Simulate a user clearing site data (including Service Workers, crucially).
  BrowsingDataRemover* remover =
      BrowsingDataRemover::CreateForUnboundedRange(GetBrowser()->profile());
  BrowsingDataRemoverCompletionObserver observer(remover);
  remover->Remove(BrowsingDataRemover::REMOVE_SITE_DATA,
                  BrowsingDataHelper::UNPROTECTED_WEB);
  observer.BlockUntilCompletion();
  // BrowsingDataRemover deletes itself.

  base::RunLoop run_loop;
  push_service()->SetContentSettingChangedCallbackForTesting(
      run_loop.QuitClosure());

  // This shouldn't (asynchronously) cause a DCHECK.
  // TODO(johnme): Get this test running on Android, which has a different
  // codepath due to sender_id being required for unsubscribeing there.
  GetBrowser()->profile()->GetHostContentSettingsMap()->
      ClearSettingsForOneType(CONTENT_SETTINGS_TYPE_PUSH_MESSAGING);

  run_loop.Run();

  // app_id should no longer be stored in prefs
  PushMessagingApplicationId stored_app_id2 = PushMessagingApplicationId::Get(
      GetBrowser()->profile(), app_id.app_id_guid());
  EXPECT_FALSE(stored_app_id2.IsValid());
}

class PushMessagingIncognitoBrowserTest : public PushMessagingBrowserTest {
 public:
  ~PushMessagingIncognitoBrowserTest() override {}

  // PushMessagingBrowserTest:
  void SetUpOnMainThread() override {
    incognito_browser_ = CreateIncognitoBrowser();
    PushMessagingBrowserTest::SetUpOnMainThread();
  }

  Browser* GetBrowser() const override { return incognito_browser_; }

 private:
  Browser* incognito_browser_ = nullptr;
};

// Regression test for https://crbug.com/476474
IN_PROC_BROWSER_TEST_F(PushMessagingIncognitoBrowserTest,
                       IncognitoGetSubscriptionDoesNotHang) {
  ASSERT_TRUE(GetBrowser()->profile()->IsOffTheRecord());

  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  // In Incognito mode the promise returned by getSubscription should not hang,
  // it should just fulfill with null.
  ASSERT_TRUE(RunScript("hasSubscription()", &script_result));
  ASSERT_EQ("false - not subscribed", script_result);
}
