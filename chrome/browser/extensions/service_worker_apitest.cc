// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/logging.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/gcm/fake_gcm_profile_service.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/notifications/desktop_notification_profile_util.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/notifications/stub_notification_display_service.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/permissions/permission_result.h"
#include "chrome/browser/push_messaging/push_messaging_app_identifier.h"
#include "chrome/browser/push_messaging/push_messaging_service_factory.h"
#include "chrome/browser/push_messaging/push_messaging_service_impl.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/gcm_driver/instance_id/fake_gcm_driver_for_instance_id.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/page_type.h"
#include "content/public/test/background_sync_test_util.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/service_worker_test_helpers.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/process_manager.h"
#include "extensions/test/background_page_watcher.h"
#include "extensions/test/extension_test_message_listener.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/url_constants.h"

#include "extensions/browser/extension_service_worker_message_filter.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "extensions/common/extension_messages.h"
#include "content/public/browser/render_process_host.h"

namespace extensions {

namespace {

// Pass into ServiceWorkerTest::StartTestFromBackgroundPage to indicate that
// registration is expected to succeed.
std::string* const kExpectSuccess = nullptr;

void DoNothingWithBool(bool b) {}

//class TestExtensionServiceWorkerMessageFilter : public ExtensionServiceWorkerMessageFilter {
class TestExtensionServiceWorkerMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit TestExtensionServiceWorkerMessageFilter(
      content::RenderProcessHost* host)
      : content::BrowserMessageFilter(ExtensionWorkerMsgStart),
        host_(host) {
    //DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  }

  TestExtensionServiceWorkerMessageFilter()
      : content::BrowserMessageFilter(ExtensionWorkerMsgStart) {
  }

  bool OnMessageReceived(const IPC::Message& message) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    if (message.type() == ExtensionHostMsg_DecrementServiceWorkerActivity::ID) {
      ++seen_num_messages_;
      if (seen_num_messages_ == expected_num_messages_) {
        std::move(quit_closure_).Run();
      }
    }
    return false;
  }

  void WaitForMessageCount(size_t count) {
    DCHECK_LE(seen_num_messages_, count);
    if (seen_num_messages_ == count)
      return;
    expected_num_messages_ = count;
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  ~TestExtensionServiceWorkerMessageFilter() override {}

  content::RenderProcessHost* host() { return host_; }

  size_t seen_num_messages_ = 0;
  size_t expected_num_messages_ = 0;
  base::OnceClosure quit_closure_;

  content::RenderProcessHost* host_;
  DISALLOW_COPY_AND_ASSIGN(TestExtensionServiceWorkerMessageFilter);
};

class ScopedBrowserClientOverride : public ChromeContentBrowserClient {
 public:
  ScopedBrowserClientOverride()
      : original_browser_client_(
          content::SetBrowserClientForTesting(this)) {
    //TestBrowserClientForServiceWorker browser_client;
    //content::ContentBrowserClient* old_browser_client =
    //    content::SetBrowserClientForTesting(this);
  }
  ~ScopedBrowserClientOverride() override {
    content::SetBrowserClientForTesting(original_browser_client_);
  }

  void WaitForMessageCount(size_t count) {
    filter_for_test_->WaitForMessageCount(count);
  }

  // ContentBrowserClient:
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override {
    filter_for_test_ = new TestExtensionServiceWorkerMessageFilter();
    host->AddFilter(filter_for_test_.get());
    ChromeContentBrowserClient::RenderProcessWillLaunch(host);
  }

 private:
  content::ContentBrowserClient* const original_browser_client_;
  scoped_refptr<TestExtensionServiceWorkerMessageFilter> filter_for_test_;
  DISALLOW_COPY_AND_ASSIGN(ScopedBrowserClientOverride);
};

class TestBrowserClientForServiceWorker : public ChromeContentBrowserClient {
 public:
  TestBrowserClientForServiceWorker() = default;

  // ChromeContentBrowserClient:
  void RenderProcessWillLaunch(
      content::RenderProcessHost* process_host) override {
    printf("->Test::RenderProcessWillLaunch\n");
    filters_.push_back(new TestExtensionServiceWorkerMessageFilter(process_host));
    process_host->AddFilter(filters_.back().get());
    ChromeContentBrowserClient::RenderProcessWillLaunch(process_host);
  }
 private:
  std::vector<scoped_refptr<TestExtensionServiceWorkerMessageFilter>> filters_;

  DISALLOW_COPY_AND_ASSIGN(TestBrowserClientForServiceWorker);
};


// Returns the newly added WebContents.
content::WebContents* AddTab(Browser* browser, const GURL& url) {
  int starting_tab_count = browser->tab_strip_model()->count();
  ui_test_utils::NavigateToURLWithDisposition(
      browser, url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  int tab_count = browser->tab_strip_model()->count();
  EXPECT_EQ(starting_tab_count + 1, tab_count);
  return browser->tab_strip_model()->GetActiveWebContents();
}

class WebContentsLoadStopObserver : content::WebContentsObserver {
 public:
  explicit WebContentsLoadStopObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        load_stop_observed_(false) {}

  void WaitForLoadStop() {
    if (load_stop_observed_)
      return;
    message_loop_runner_ = new content::MessageLoopRunner;
    message_loop_runner_->Run();
  }

 private:
  void DidStopLoading() override {
    load_stop_observed_ = true;
    if (message_loop_runner_)
      message_loop_runner_->Quit();
  }

  bool load_stop_observed_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsLoadStopObserver);
};

}  // namespace

class ServiceWorkerTest : public ExtensionApiTest {
 public:
  ServiceWorkerTest() : current_channel_(version_info::Channel::STABLE) {}

  ~ServiceWorkerTest() override {}

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("a.com", "127.0.0.1");
  }

 protected:
  // Returns the ProcessManager for the test's profile.
  ProcessManager* process_manager() { return ProcessManager::Get(profile()); }

  // Starts running a test from the background page test extension.
  //
  // This registers a service worker with |script_name|, and fetches the
  // registration result.
  //
  // If |error_or_null| is null (kExpectSuccess), success is expected and this
  // will fail if there is an error.
  // If |error_or_null| is not null, nothing is assumed, and the error (which
  // may be empty) is written to it.
  const Extension* StartTestFromBackgroundPage(const char* script_name,
                                               std::string* error_or_null) {
    const Extension* extension =
        LoadExtension(test_data_dir_.AppendASCII("service_worker/background"));
    CHECK(extension);
    ExtensionHost* background_host =
        process_manager()->GetBackgroundHostForExtension(extension->id());
    CHECK(background_host);
    std::string error;
    CHECK(content::ExecuteScriptAndExtractString(
        background_host->host_contents(),
        base::StringPrintf("test.registerServiceWorker('%s')", script_name),
        &error));
    if (error_or_null)
      *error_or_null = error;
    else if (!error.empty())
      ADD_FAILURE() << "Got unexpected error " << error;
    return extension;
  }

  // Navigates the browser to a new tab at |url|, waits for it to load, then
  // returns it.
  content::WebContents* Navigate(const GURL& url) {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::WaitForLoadStop(web_contents);
    return web_contents;
  }

  // Navigates the browser to |url| and returns the new tab's page type.
  content::PageType NavigateAndGetPageType(const GURL& url) {
    return Navigate(url)->GetController().GetActiveEntry()->GetPageType();
  }

  // Extracts the innerText from |contents|.
  std::string ExtractInnerText(content::WebContents* contents) {
    std::string inner_text;
    if (!content::ExecuteScriptAndExtractString(
            contents,
            "window.domAutomationController.send(document.body.innerText)",
            &inner_text)) {
      ADD_FAILURE() << "Failed to get inner text for "
                    << contents->GetVisibleURL();
    }
    return inner_text;
  }

  // Navigates the browser to |url|, then returns the innerText of the new
  // tab's WebContents' main frame.
  std::string NavigateAndExtractInnerText(const GURL& url) {
    return ExtractInnerText(Navigate(url));
  }

  size_t GetWorkerRefCount(const GURL& origin) {
    content::ServiceWorkerContext* sw_context =
        content::BrowserContext::GetDefaultStoragePartition(
            browser()->profile())
            ->GetServiceWorkerContext();
    base::RunLoop run_loop;
    size_t ref_count = 0;
    auto set_ref_count = [](size_t* ref_count, base::RunLoop* run_loop,
                            size_t external_request_count) {
      *ref_count = external_request_count;
      run_loop->Quit();
    };
    sw_context->CountExternalRequestsForTest(
        origin, base::Bind(set_ref_count, &ref_count, &run_loop));
    run_loop.Run();
    return ref_count;
  }

 private:
  // Sets the channel to "stable".
  // Not useful after we've opened extension Service Workers to stable
  // channel.
  // TODO(lazyboy): Remove this when ExtensionServiceWorkersEnabled() is
  // removed.
  ScopedCurrentChannel current_channel_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerTest);
};

class ServiceWorkerBackgroundSyncTest : public ServiceWorkerTest {
 public:
  ServiceWorkerBackgroundSyncTest() {}
  ~ServiceWorkerBackgroundSyncTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // ServiceWorkerRegistration.sync requires experimental flag.
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
    ServiceWorkerTest::SetUpCommandLine(command_line);
  }

  void SetUp() override {
    content::background_sync_test_util::SetIgnoreNetworkChangeNotifier(true);
    ServiceWorkerTest::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerBackgroundSyncTest);
};

class ServiceWorkerPushMessagingTest : public ServiceWorkerTest {
 public:
  ServiceWorkerPushMessagingTest()
      : gcm_driver_(nullptr), push_service_(nullptr) {}
  ~ServiceWorkerPushMessagingTest() override {}

  void GrantNotificationPermissionForTest(const GURL& url) {
    GURL origin = url.GetOrigin();
    DesktopNotificationProfileUtil::GrantPermission(profile(), origin);
    ASSERT_EQ(CONTENT_SETTING_ALLOW,
              PermissionManager::Get(profile())
                  ->GetPermissionStatus(CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
                                        origin, origin)
                  .content_setting);
  }

  PushMessagingAppIdentifier GetAppIdentifierForServiceWorkerRegistration(
      int64_t service_worker_registration_id,
      const GURL& origin) {
    PushMessagingAppIdentifier app_identifier =
        PushMessagingAppIdentifier::FindByServiceWorker(
            profile(), origin, service_worker_registration_id);

    EXPECT_FALSE(app_identifier.is_null());
    return app_identifier;
  }

  // ExtensionApiTest overrides.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
    ServiceWorkerTest::SetUpCommandLine(command_line);
  }
  void SetUpOnMainThread() override {
    NotificationDisplayServiceFactory::GetInstance()->SetTestingFactory(
        profile(), &StubNotificationDisplayService::FactoryForTests);

    gcm::FakeGCMProfileService* gcm_service =
        static_cast<gcm::FakeGCMProfileService*>(
            gcm::GCMProfileServiceFactory::GetInstance()
                ->SetTestingFactoryAndUse(profile(),
                                          &gcm::FakeGCMProfileService::Build));
    gcm_driver_ = static_cast<instance_id::FakeGCMDriverForInstanceID*>(
        gcm_service->driver());
    push_service_ = PushMessagingServiceFactory::GetForProfile(profile());

    ServiceWorkerTest::SetUpOnMainThread();
  }

  instance_id::FakeGCMDriverForInstanceID* gcm_driver() const {
    return gcm_driver_;
  }
  PushMessagingServiceImpl* push_service() const { return push_service_; }

 private:
  instance_id::FakeGCMDriverForInstanceID* gcm_driver_;
  PushMessagingServiceImpl* push_service_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPushMessagingTest);
};

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, RegisterSucceeds) {
  StartTestFromBackgroundPage("register.js", kExpectSuccess);
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, UpdateRefreshesServiceWorker) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::ScopedTempDir scoped_temp_dir;
  ASSERT_TRUE(scoped_temp_dir.CreateUniqueTempDir());
  base::FilePath pem_path = test_data_dir_.AppendASCII("service_worker")
                                .AppendASCII("update")
                                .AppendASCII("service_worker.pem");
  base::FilePath path_v1 =
      PackExtensionWithOptions(test_data_dir_.AppendASCII("service_worker")
                                   .AppendASCII("update")
                                   .AppendASCII("v1"),
                               scoped_temp_dir.GetPath().AppendASCII("v1.crx"),
                               pem_path, base::FilePath());
  base::FilePath path_v2 =
      PackExtensionWithOptions(test_data_dir_.AppendASCII("service_worker")
                                   .AppendASCII("update")
                                   .AppendASCII("v2"),
                               scoped_temp_dir.GetPath().AppendASCII("v2.crx"),
                               pem_path, base::FilePath());
  const char* kId = "hfaanndiiilofhfokeanhddpkfffchdi";

  ExtensionTestMessageListener listener_v1("Pong from version 1", false);
  listener_v1.set_failure_message("FAILURE_V1");
  // Install version 1.0 of the extension.
  ASSERT_TRUE(InstallExtension(path_v1, 1));
  EXPECT_TRUE(extensions::ExtensionRegistry::Get(profile())
                  ->enabled_extensions()
                  .GetByID(kId));
  EXPECT_TRUE(listener_v1.WaitUntilSatisfied());

  ExtensionTestMessageListener listener_v2("Pong from version 2", false);
  listener_v2.set_failure_message("FAILURE_V2");

  // Update to version 2.0.
  EXPECT_TRUE(UpdateExtension(kId, path_v2, 0));
  EXPECT_TRUE(extensions::ExtensionRegistry::Get(profile())
                  ->enabled_extensions()
                  .GetByID(kId));
  EXPECT_TRUE(listener_v2.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, UpdateWithoutSkipWaiting) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::ScopedTempDir scoped_temp_dir;
  ASSERT_TRUE(scoped_temp_dir.CreateUniqueTempDir());
  base::FilePath pem_path = test_data_dir_.AppendASCII("service_worker")
                                .AppendASCII("update_without_skip_waiting")
                                .AppendASCII("update_without_skip_waiting.pem");
  base::FilePath path_v1 =
      PackExtensionWithOptions(test_data_dir_.AppendASCII("service_worker")
                                   .AppendASCII("update_without_skip_waiting")
                                   .AppendASCII("v1"),
                               scoped_temp_dir.GetPath().AppendASCII("v1.crx"),
                               pem_path, base::FilePath());
  base::FilePath path_v2 =
      PackExtensionWithOptions(test_data_dir_.AppendASCII("service_worker")
                                   .AppendASCII("update_without_skip_waiting")
                                   .AppendASCII("v2"),
                               scoped_temp_dir.GetPath().AppendASCII("v2.crx"),
                               pem_path, base::FilePath());
  const char* kId = "mhnnnflgagdakldgjpfcofkiocpdmogl";

  // Install version 1.0 of the extension.
  ASSERT_TRUE(InstallExtension(path_v1, 1));
  EXPECT_TRUE(extensions::ExtensionRegistry::Get(profile())
                  ->enabled_extensions()
                  .GetByID(kId));
  const Extension* extension = extensions::ExtensionRegistry::Get(profile())
                                   ->enabled_extensions()
                                   .GetByID(kId);

  ExtensionTestMessageListener listener1("Pong from version 1", false);
  listener1.set_failure_message("FAILURE");
  content::WebContents* web_contents =
      AddTab(browser(), extension->GetResourceURL("page.html"));
  EXPECT_TRUE(listener1.WaitUntilSatisfied());

  // Update to version 2.0.
  EXPECT_TRUE(UpdateExtension(kId, path_v2, 0));
  EXPECT_TRUE(extensions::ExtensionRegistry::Get(profile())
                  ->enabled_extensions()
                  .GetByID(kId));
  const Extension* extension_after_update =
      extensions::ExtensionRegistry::Get(profile())
          ->enabled_extensions()
          .GetByID(kId);

  // Service worker version 2 would be installed but it won't be controlling
  // the extension page yet.
  ExtensionTestMessageListener listener2("Pong from version 1", false);
  listener2.set_failure_message("FAILURE");
  web_contents =
      AddTab(browser(), extension_after_update->GetResourceURL("page.html"));
  EXPECT_TRUE(listener2.WaitUntilSatisfied());

  // Navigate the tab away from the extension page so that no clients are
  // using the service worker.
  // Note that just closing the tab with WebContentsDestroyedWatcher doesn't
  // seem to be enough because it returns too early.
  WebContentsLoadStopObserver navigate_away_observer(web_contents);
  web_contents->GetController().LoadURL(
      GURL(url::kAboutBlankURL), content::Referrer(), ui::PAGE_TRANSITION_TYPED,
      std::string());
  navigate_away_observer.WaitForLoadStop();

  // Now expect service worker version 2 to control the extension page.
  ExtensionTestMessageListener listener3("Pong from version 2", false);
  listener3.set_failure_message("FAILURE");
  web_contents =
      AddTab(browser(), extension_after_update->GetResourceURL("page.html"));
  EXPECT_TRUE(listener3.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, FetchArbitraryPaths) {
  const Extension* extension =
      StartTestFromBackgroundPage("fetch.js", kExpectSuccess);

  // Open some arbirary paths. Their contents should be what the service worker
  // responds with, which in this case is the path of the fetch.
  EXPECT_EQ(
      "Caught a fetch for /index.html",
      NavigateAndExtractInnerText(extension->GetResourceURL("index.html")));
  EXPECT_EQ("Caught a fetch for /path/to/other.html",
            NavigateAndExtractInnerText(
                extension->GetResourceURL("path/to/other.html")));
  EXPECT_EQ("Caught a fetch for /some/text/file.txt",
            NavigateAndExtractInnerText(
                extension->GetResourceURL("some/text/file.txt")));
  EXPECT_EQ("Caught a fetch for /no/file/extension",
            NavigateAndExtractInnerText(
                extension->GetResourceURL("no/file/extension")));
  EXPECT_EQ("Caught a fetch for /",
            NavigateAndExtractInnerText(extension->GetResourceURL("")));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, SWServedBackgroundPageReceivesEvent) {
  const Extension* extension =
      StartTestFromBackgroundPage("replace_background.js", kExpectSuccess);
  ExtensionHost* background_page =
      process_manager()->GetBackgroundHostForExtension(extension->id());
  ASSERT_TRUE(background_page);

  // Close the background page and start it again so that the service worker
  // will start controlling pages.
  background_page->Close();
  BackgroundPageWatcher(process_manager(), extension).WaitForClose();
  background_page = nullptr;
  process_manager()->WakeEventPage(extension->id(),
                                   base::Bind(&DoNothingWithBool));
  BackgroundPageWatcher(process_manager(), extension).WaitForOpen();

  // Since the SW is now controlling the extension, the SW serves the background
  // script. page.html sends a message to the background script and we verify
  // that the SW served background script correctly receives the message/event.
  ExtensionTestMessageListener listener("onMessage/SW BG.", false);
  listener.set_failure_message("onMessage/original BG.");
  content::WebContents* web_contents =
      AddTab(browser(), extension->GetResourceURL("page.html"));
  ASSERT_TRUE(web_contents);
  EXPECT_TRUE(listener.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest,
                       LoadingBackgroundPageBypassesServiceWorker) {
  const Extension* extension =
      StartTestFromBackgroundPage("fetch.js", kExpectSuccess);

  std::string kExpectedInnerText = "background.html contents for testing.";

  // Sanity check that the background page has the expected content.
  ExtensionHost* background_page =
      process_manager()->GetBackgroundHostForExtension(extension->id());
  ASSERT_TRUE(background_page);
  EXPECT_EQ(kExpectedInnerText,
            ExtractInnerText(background_page->host_contents()));

  // Close the background page.
  background_page->Close();
  BackgroundPageWatcher(process_manager(), extension).WaitForClose();
  background_page = nullptr;

  // Start it again.
  process_manager()->WakeEventPage(extension->id(),
                                   base::Bind(&DoNothingWithBool));
  BackgroundPageWatcher(process_manager(), extension).WaitForOpen();

  // Content should not have been affected by the fetch, which would otherwise
  // be "Caught fetch for...".
  background_page =
      process_manager()->GetBackgroundHostForExtension(extension->id());
  ASSERT_TRUE(background_page);
  content::WaitForLoadStop(background_page->host_contents());

  // TODO(kalman): Everything you've read has been a LIE! It should be:
  //
  // EXPECT_EQ(kExpectedInnerText,
  //           ExtractInnerText(background_page->host_contents()));
  //
  // but there is a bug, and we're actually *not* bypassing the service worker
  // for background page loads! For now, let it pass (assert wrong behavior)
  // because it's not a regression, but this must be fixed eventually.
  //
  // Tracked in crbug.com/532720.
  EXPECT_EQ("Caught a fetch for /background.html",
            ExtractInnerText(background_page->host_contents()));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest,
                       ServiceWorkerPostsMessageToBackgroundClient) {
  const Extension* extension = StartTestFromBackgroundPage(
      "post_message_to_background_client.js", kExpectSuccess);

  // The service worker in this test simply posts a message to the background
  // client it receives from getBackgroundClient().
  const char* kScript =
      "var messagePromise = null;\n"
      "if (test.lastMessageFromServiceWorker) {\n"
      "  messagePromise = Promise.resolve(test.lastMessageFromServiceWorker);\n"
      "} else {\n"
      "  messagePromise = test.waitForMessage(navigator.serviceWorker);\n"
      "}\n"
      "messagePromise.then(function(message) {\n"
      "  window.domAutomationController.send(String(message == 'success'));\n"
      "})\n";
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(extension->id(), kScript));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest,
                       BackgroundPagePostsMessageToServiceWorker) {
  const Extension* extension =
      StartTestFromBackgroundPage("post_message_to_sw.js", kExpectSuccess);

  // The service worker in this test waits for a message, then echoes it back
  // by posting a message to the background page via getBackgroundClient().
  const char* kScript =
      "var mc = new MessageChannel();\n"
      "test.waitForMessage(mc.port1).then(function(message) {\n"
      "  window.domAutomationController.send(String(message == 'hello'));\n"
      "});\n"
      "test.registeredServiceWorker.postMessage(\n"
      "    {message: 'hello', port: mc.port2}, [mc.port2])\n";
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(extension->id(), kScript));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest,
                       ServiceWorkerSuspensionOnExtensionUnload) {
  // For this test, only hold onto the extension's ID and URL + a function to
  // get a resource URL, because we're going to be disabling and uninstalling
  // it, which will invalidate the pointer.
  std::string extension_id;
  GURL extension_url;
  {
    const Extension* extension =
        StartTestFromBackgroundPage("fetch.js", kExpectSuccess);
    extension_id = extension->id();
    extension_url = extension->url();
  }
  auto get_resource_url = [&extension_url](const std::string& path) {
    return Extension::GetResourceURL(extension_url, path);
  };

  // Fetch should route to the service worker.
  EXPECT_EQ("Caught a fetch for /index.html",
            NavigateAndExtractInnerText(get_resource_url("index.html")));

  // Disable the extension. Opening the page should fail.
  extension_service()->DisableExtension(extension_id,
                                        Extension::DISABLE_USER_ACTION);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(content::PAGE_TYPE_ERROR,
            NavigateAndGetPageType(get_resource_url("index.html")));
  EXPECT_EQ(content::PAGE_TYPE_ERROR,
            NavigateAndGetPageType(get_resource_url("other.html")));

  // Re-enable the extension. Opening pages should immediately start to succeed
  // again.
  extension_service()->EnableExtension(extension_id);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("Caught a fetch for /index.html",
            NavigateAndExtractInnerText(get_resource_url("index.html")));
  EXPECT_EQ("Caught a fetch for /other.html",
            NavigateAndExtractInnerText(get_resource_url("other.html")));
  EXPECT_EQ("Caught a fetch for /another.html",
            NavigateAndExtractInnerText(get_resource_url("another.html")));

  // Uninstall the extension. Opening pages should fail again.
  base::string16 error;
  extension_service()->UninstallExtension(
      extension_id, UninstallReason::UNINSTALL_REASON_FOR_TESTING,
      base::Bind(&base::DoNothing), &error);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(content::PAGE_TYPE_ERROR,
            NavigateAndGetPageType(get_resource_url("index.html")));
  EXPECT_EQ(content::PAGE_TYPE_ERROR,
            NavigateAndGetPageType(get_resource_url("other.html")));
  EXPECT_EQ(content::PAGE_TYPE_ERROR,
            NavigateAndGetPageType(get_resource_url("anotherother.html")));
  EXPECT_EQ(content::PAGE_TYPE_ERROR,
            NavigateAndGetPageType(get_resource_url("final.html")));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, BackgroundPageIsWokenIfAsleep) {
  const Extension* extension =
      StartTestFromBackgroundPage("wake_on_fetch.js", kExpectSuccess);

  // Navigate to special URLs that this test's service worker recognises, each
  // making a check then populating the response with either "true" or "false".
  EXPECT_EQ("true", NavigateAndExtractInnerText(extension->GetResourceURL(
                        "background-client-is-awake")));
  EXPECT_EQ("true", NavigateAndExtractInnerText(
                        extension->GetResourceURL("ping-background-client")));
  // Ping more than once for good measure.
  EXPECT_EQ("true", NavigateAndExtractInnerText(
                        extension->GetResourceURL("ping-background-client")));

  // Shut down the event page. The SW should detect that it's closed, but still
  // be able to ping it.
  ExtensionHost* background_page =
      process_manager()->GetBackgroundHostForExtension(extension->id());
  ASSERT_TRUE(background_page);
  background_page->Close();
  BackgroundPageWatcher(process_manager(), extension).WaitForClose();

  EXPECT_EQ("false", NavigateAndExtractInnerText(extension->GetResourceURL(
                         "background-client-is-awake")));
  EXPECT_EQ("true", NavigateAndExtractInnerText(
                        extension->GetResourceURL("ping-background-client")));
  EXPECT_EQ("true", NavigateAndExtractInnerText(
                        extension->GetResourceURL("ping-background-client")));
  EXPECT_EQ("true", NavigateAndExtractInnerText(extension->GetResourceURL(
                        "background-client-is-awake")));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest,
                       GetBackgroundClientFailsWithNoBackgroundPage) {
  // This extension doesn't have a background page, only a tab at page.html.
  // The service worker it registers tries to call getBackgroundClient() and
  // should fail.
  // Note that this also tests that service workers can be registered from tabs.
  EXPECT_TRUE(RunExtensionSubtest("service_worker/no_background", "page.html"));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, NotificationAPI) {
  EXPECT_TRUE(RunExtensionSubtest("service_worker/notifications/has_permission",
                                  "page.html"));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, WebAccessibleResourcesFetch) {
  EXPECT_TRUE(RunExtensionSubtest(
      "service_worker/web_accessible_resources/fetch/", "page.html"));
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, TabsCreate) {
  // Extensions APIs from SW are only enabled on trunk.
  ScopedCurrentChannel current_channel_override(version_info::Channel::UNKNOWN);
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII("service_worker/tabs_create"), kFlagNone);
  ASSERT_TRUE(extension);
  ui_test_utils::NavigateToURL(browser(),
                               extension->GetResourceURL("page.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  int starting_tab_count = browser()->tab_strip_model()->count();
  std::string result;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, "window.runServiceWorker()", &result));
  ASSERT_EQ("chrome.tabs.create callback", result);
  EXPECT_EQ(starting_tab_count + 1, browser()->tab_strip_model()->count());

  // Check extension shutdown path.
  UnloadExtension(extension->id());
  EXPECT_EQ(starting_tab_count, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, Events) {
  // Extensions APIs from SW are only enabled on trunk.
  ScopedCurrentChannel current_channel_override(version_info::Channel::UNKNOWN);
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII("service_worker/events"), kFlagNone);
  ASSERT_TRUE(extension);
  ui_test_utils::NavigateToURL(browser(),
                               extension->GetResourceURL("page.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  std::string result;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, "window.runEventTest()", &result));
  ASSERT_EQ("chrome.tabs.onUpdated callback", result);
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, EventsToStoppedWorker) {
  // Extensions APIs from SW are only enabled on trunk.
  ScopedCurrentChannel current_channel_override(version_info::Channel::UNKNOWN);
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII("service_worker/events_to_stopped_worker"),
      kFlagNone);
  ASSERT_TRUE(extension);
  ui_test_utils::NavigateToURL(browser(),
                               extension->GetResourceURL("page.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  {
    std::string result;
    ASSERT_TRUE(content::ExecuteScriptAndExtractString(
        web_contents, "window.runServiceWorker()", &result));
    ASSERT_EQ("ready", result);

    base::RunLoop run_loop;
    content::StoragePartition* storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(
            browser()->profile());
    content::StopServiceWorkerForPattern(
        storage_partition->GetServiceWorkerContext(),
        // The service worker is registered at the top level scope.
        extension->url(), run_loop.QuitClosure());
    run_loop.Run();
  }

  std::string result;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, "window.createTabThenUpdate()", &result));
  ASSERT_EQ("chrome.tabs.onUpdated callback", result);
}

#define REPEAT_F(c, n, b) \
IN_PROC_BROWSER_TEST_F(c, n##000) b \
IN_PROC_BROWSER_TEST_F(c, n##001) b \
IN_PROC_BROWSER_TEST_F(c, n##002) b \
IN_PROC_BROWSER_TEST_F(c, n##003) b \
IN_PROC_BROWSER_TEST_F(c, n##004) b \
IN_PROC_BROWSER_TEST_F(c, n##005) b \
IN_PROC_BROWSER_TEST_F(c, n##006) b \
IN_PROC_BROWSER_TEST_F(c, n##007) b \
IN_PROC_BROWSER_TEST_F(c, n##008) b \
IN_PROC_BROWSER_TEST_F(c, n##009) b \
IN_PROC_BROWSER_TEST_F(c, n##010) b \
IN_PROC_BROWSER_TEST_F(c, n##011) b \
IN_PROC_BROWSER_TEST_F(c, n##012) b \
IN_PROC_BROWSER_TEST_F(c, n##013) b \
IN_PROC_BROWSER_TEST_F(c, n##014) b \
IN_PROC_BROWSER_TEST_F(c, n##015) b \
IN_PROC_BROWSER_TEST_F(c, n##016) b \
IN_PROC_BROWSER_TEST_F(c, n##017) b \
IN_PROC_BROWSER_TEST_F(c, n##018) b \
IN_PROC_BROWSER_TEST_F(c, n##019) b \
IN_PROC_BROWSER_TEST_F(c, n##020) b \
IN_PROC_BROWSER_TEST_F(c, n##021) b \
IN_PROC_BROWSER_TEST_F(c, n##022) b \
IN_PROC_BROWSER_TEST_F(c, n##023) b \
IN_PROC_BROWSER_TEST_F(c, n##024) b \
IN_PROC_BROWSER_TEST_F(c, n##025) b \
IN_PROC_BROWSER_TEST_F(c, n##026) b \
IN_PROC_BROWSER_TEST_F(c, n##027) b \
IN_PROC_BROWSER_TEST_F(c, n##028) b \
IN_PROC_BROWSER_TEST_F(c, n##029) b \
IN_PROC_BROWSER_TEST_F(c, n##030) b \
IN_PROC_BROWSER_TEST_F(c, n##031) b \
IN_PROC_BROWSER_TEST_F(c, n##032) b \
IN_PROC_BROWSER_TEST_F(c, n##033) b \
IN_PROC_BROWSER_TEST_F(c, n##034) b \
IN_PROC_BROWSER_TEST_F(c, n##035) b \
IN_PROC_BROWSER_TEST_F(c, n##036) b \
IN_PROC_BROWSER_TEST_F(c, n##037) b \
IN_PROC_BROWSER_TEST_F(c, n##038) b \
IN_PROC_BROWSER_TEST_F(c, n##039) b \
IN_PROC_BROWSER_TEST_F(c, n##040) b \
IN_PROC_BROWSER_TEST_F(c, n##041) b \
IN_PROC_BROWSER_TEST_F(c, n##042) b \
IN_PROC_BROWSER_TEST_F(c, n##043) b \
IN_PROC_BROWSER_TEST_F(c, n##044) b \
IN_PROC_BROWSER_TEST_F(c, n##045) b \
IN_PROC_BROWSER_TEST_F(c, n##046) b \
IN_PROC_BROWSER_TEST_F(c, n##047) b \
IN_PROC_BROWSER_TEST_F(c, n##048) b \
IN_PROC_BROWSER_TEST_F(c, n##049) b \
IN_PROC_BROWSER_TEST_F(c, n##050) b \
IN_PROC_BROWSER_TEST_F(c, n##051) b \
IN_PROC_BROWSER_TEST_F(c, n##052) b \
IN_PROC_BROWSER_TEST_F(c, n##053) b \
IN_PROC_BROWSER_TEST_F(c, n##054) b \
IN_PROC_BROWSER_TEST_F(c, n##055) b \
IN_PROC_BROWSER_TEST_F(c, n##056) b \
IN_PROC_BROWSER_TEST_F(c, n##057) b \
IN_PROC_BROWSER_TEST_F(c, n##058) b \
IN_PROC_BROWSER_TEST_F(c, n##059) b \
IN_PROC_BROWSER_TEST_F(c, n##060) b \
IN_PROC_BROWSER_TEST_F(c, n##061) b \
IN_PROC_BROWSER_TEST_F(c, n##062) b \
IN_PROC_BROWSER_TEST_F(c, n##063) b \
IN_PROC_BROWSER_TEST_F(c, n##064) b \
IN_PROC_BROWSER_TEST_F(c, n##065) b \
IN_PROC_BROWSER_TEST_F(c, n##066) b \
IN_PROC_BROWSER_TEST_F(c, n##067) b \
IN_PROC_BROWSER_TEST_F(c, n##068) b \
IN_PROC_BROWSER_TEST_F(c, n##069) b \
IN_PROC_BROWSER_TEST_F(c, n##070) b \
IN_PROC_BROWSER_TEST_F(c, n##071) b \
IN_PROC_BROWSER_TEST_F(c, n##072) b \
IN_PROC_BROWSER_TEST_F(c, n##073) b \
IN_PROC_BROWSER_TEST_F(c, n##074) b \
IN_PROC_BROWSER_TEST_F(c, n##075) b \
IN_PROC_BROWSER_TEST_F(c, n##076) b \
IN_PROC_BROWSER_TEST_F(c, n##077) b \
IN_PROC_BROWSER_TEST_F(c, n##078) b \
IN_PROC_BROWSER_TEST_F(c, n##079) b \
IN_PROC_BROWSER_TEST_F(c, n##080) b \
IN_PROC_BROWSER_TEST_F(c, n##081) b \
IN_PROC_BROWSER_TEST_F(c, n##082) b \
IN_PROC_BROWSER_TEST_F(c, n##083) b \
IN_PROC_BROWSER_TEST_F(c, n##084) b \
IN_PROC_BROWSER_TEST_F(c, n##085) b \
IN_PROC_BROWSER_TEST_F(c, n##086) b \
IN_PROC_BROWSER_TEST_F(c, n##087) b \
IN_PROC_BROWSER_TEST_F(c, n##088) b \
IN_PROC_BROWSER_TEST_F(c, n##089) b \
IN_PROC_BROWSER_TEST_F(c, n##090) b \
IN_PROC_BROWSER_TEST_F(c, n##091) b \
IN_PROC_BROWSER_TEST_F(c, n##092) b \
IN_PROC_BROWSER_TEST_F(c, n##093) b \
IN_PROC_BROWSER_TEST_F(c, n##094) b \
IN_PROC_BROWSER_TEST_F(c, n##095) b \
IN_PROC_BROWSER_TEST_F(c, n##096) b \
IN_PROC_BROWSER_TEST_F(c, n##097) b \
IN_PROC_BROWSER_TEST_F(c, n##098) b \
IN_PROC_BROWSER_TEST_F(c, n##099) b \
IN_PROC_BROWSER_TEST_F(c, n##100) b \
IN_PROC_BROWSER_TEST_F(c, n##101) b \
IN_PROC_BROWSER_TEST_F(c, n##102) b \
IN_PROC_BROWSER_TEST_F(c, n##103) b \
IN_PROC_BROWSER_TEST_F(c, n##104) b \
IN_PROC_BROWSER_TEST_F(c, n##105) b \
IN_PROC_BROWSER_TEST_F(c, n##106) b \
IN_PROC_BROWSER_TEST_F(c, n##107) b \
IN_PROC_BROWSER_TEST_F(c, n##108) b \
IN_PROC_BROWSER_TEST_F(c, n##109) b \
IN_PROC_BROWSER_TEST_F(c, n##110) b \
IN_PROC_BROWSER_TEST_F(c, n##111) b \
IN_PROC_BROWSER_TEST_F(c, n##112) b \
IN_PROC_BROWSER_TEST_F(c, n##113) b \
IN_PROC_BROWSER_TEST_F(c, n##114) b \
IN_PROC_BROWSER_TEST_F(c, n##115) b \
IN_PROC_BROWSER_TEST_F(c, n##116) b \
IN_PROC_BROWSER_TEST_F(c, n##117) b \
IN_PROC_BROWSER_TEST_F(c, n##118) b \
IN_PROC_BROWSER_TEST_F(c, n##119) b \
IN_PROC_BROWSER_TEST_F(c, n##120) b \
IN_PROC_BROWSER_TEST_F(c, n##121) b \
IN_PROC_BROWSER_TEST_F(c, n##122) b \
IN_PROC_BROWSER_TEST_F(c, n##123) b \
IN_PROC_BROWSER_TEST_F(c, n##124) b \
IN_PROC_BROWSER_TEST_F(c, n##125) b \
IN_PROC_BROWSER_TEST_F(c, n##126) b \
IN_PROC_BROWSER_TEST_F(c, n##127) b \
IN_PROC_BROWSER_TEST_F(c, n##128) b \
IN_PROC_BROWSER_TEST_F(c, n##129) b \
IN_PROC_BROWSER_TEST_F(c, n##130) b \
IN_PROC_BROWSER_TEST_F(c, n##131) b \
IN_PROC_BROWSER_TEST_F(c, n##132) b \
IN_PROC_BROWSER_TEST_F(c, n##133) b \
IN_PROC_BROWSER_TEST_F(c, n##134) b \
IN_PROC_BROWSER_TEST_F(c, n##135) b \
IN_PROC_BROWSER_TEST_F(c, n##136) b \
IN_PROC_BROWSER_TEST_F(c, n##137) b \
IN_PROC_BROWSER_TEST_F(c, n##138) b \
IN_PROC_BROWSER_TEST_F(c, n##139) b \
IN_PROC_BROWSER_TEST_F(c, n##140) b \
IN_PROC_BROWSER_TEST_F(c, n##141) b \
IN_PROC_BROWSER_TEST_F(c, n##142) b \
IN_PROC_BROWSER_TEST_F(c, n##143) b \
IN_PROC_BROWSER_TEST_F(c, n##144) b \
IN_PROC_BROWSER_TEST_F(c, n##145) b \
IN_PROC_BROWSER_TEST_F(c, n##146) b \
IN_PROC_BROWSER_TEST_F(c, n##147) b \
IN_PROC_BROWSER_TEST_F(c, n##148) b \
IN_PROC_BROWSER_TEST_F(c, n##149) b \
IN_PROC_BROWSER_TEST_F(c, n##150) b \
IN_PROC_BROWSER_TEST_F(c, n##151) b \
IN_PROC_BROWSER_TEST_F(c, n##152) b \
IN_PROC_BROWSER_TEST_F(c, n##153) b \
IN_PROC_BROWSER_TEST_F(c, n##154) b \
IN_PROC_BROWSER_TEST_F(c, n##155) b \
IN_PROC_BROWSER_TEST_F(c, n##156) b \
IN_PROC_BROWSER_TEST_F(c, n##157) b \
IN_PROC_BROWSER_TEST_F(c, n##158) b \
IN_PROC_BROWSER_TEST_F(c, n##159) b \
IN_PROC_BROWSER_TEST_F(c, n##160) b \
IN_PROC_BROWSER_TEST_F(c, n##161) b \
IN_PROC_BROWSER_TEST_F(c, n##162) b \
IN_PROC_BROWSER_TEST_F(c, n##163) b \
IN_PROC_BROWSER_TEST_F(c, n##164) b \
IN_PROC_BROWSER_TEST_F(c, n##165) b \
IN_PROC_BROWSER_TEST_F(c, n##166) b \
IN_PROC_BROWSER_TEST_F(c, n##167) b \
IN_PROC_BROWSER_TEST_F(c, n##168) b \
IN_PROC_BROWSER_TEST_F(c, n##169) b \
IN_PROC_BROWSER_TEST_F(c, n##170) b \
IN_PROC_BROWSER_TEST_F(c, n##171) b \
IN_PROC_BROWSER_TEST_F(c, n##172) b \
IN_PROC_BROWSER_TEST_F(c, n##173) b \
IN_PROC_BROWSER_TEST_F(c, n##174) b \
IN_PROC_BROWSER_TEST_F(c, n##175) b \
IN_PROC_BROWSER_TEST_F(c, n##176) b \
IN_PROC_BROWSER_TEST_F(c, n##177) b \
IN_PROC_BROWSER_TEST_F(c, n##178) b \
IN_PROC_BROWSER_TEST_F(c, n##179) b \
IN_PROC_BROWSER_TEST_F(c, n##180) b \
IN_PROC_BROWSER_TEST_F(c, n##181) b \
IN_PROC_BROWSER_TEST_F(c, n##182) b \
IN_PROC_BROWSER_TEST_F(c, n##183) b \
IN_PROC_BROWSER_TEST_F(c, n##184) b \
IN_PROC_BROWSER_TEST_F(c, n##185) b \
IN_PROC_BROWSER_TEST_F(c, n##186) b \
IN_PROC_BROWSER_TEST_F(c, n##187) b \
IN_PROC_BROWSER_TEST_F(c, n##188) b \
IN_PROC_BROWSER_TEST_F(c, n##189) b \
IN_PROC_BROWSER_TEST_F(c, n##190) b \
IN_PROC_BROWSER_TEST_F(c, n##191) b \
IN_PROC_BROWSER_TEST_F(c, n##192) b \
IN_PROC_BROWSER_TEST_F(c, n##193) b \
IN_PROC_BROWSER_TEST_F(c, n##194) b \
IN_PROC_BROWSER_TEST_F(c, n##195) b \
IN_PROC_BROWSER_TEST_F(c, n##196) b \
IN_PROC_BROWSER_TEST_F(c, n##197) b \
IN_PROC_BROWSER_TEST_F(c, n##198) b \
IN_PROC_BROWSER_TEST_F(c, n##199) b \
IN_PROC_BROWSER_TEST_F(c, n##200) b \
IN_PROC_BROWSER_TEST_F(c, n##201) b \
IN_PROC_BROWSER_TEST_F(c, n##202) b \
IN_PROC_BROWSER_TEST_F(c, n##203) b \
IN_PROC_BROWSER_TEST_F(c, n##204) b \
IN_PROC_BROWSER_TEST_F(c, n##205) b \
IN_PROC_BROWSER_TEST_F(c, n##206) b \
IN_PROC_BROWSER_TEST_F(c, n##207) b \
IN_PROC_BROWSER_TEST_F(c, n##208) b \
IN_PROC_BROWSER_TEST_F(c, n##209) b \
IN_PROC_BROWSER_TEST_F(c, n##210) b \
IN_PROC_BROWSER_TEST_F(c, n##211) b \
IN_PROC_BROWSER_TEST_F(c, n##212) b \
IN_PROC_BROWSER_TEST_F(c, n##213) b \
IN_PROC_BROWSER_TEST_F(c, n##214) b \
IN_PROC_BROWSER_TEST_F(c, n##215) b \
IN_PROC_BROWSER_TEST_F(c, n##216) b \
IN_PROC_BROWSER_TEST_F(c, n##217) b \
IN_PROC_BROWSER_TEST_F(c, n##218) b \
IN_PROC_BROWSER_TEST_F(c, n##219) b \
IN_PROC_BROWSER_TEST_F(c, n##220) b \
IN_PROC_BROWSER_TEST_F(c, n##221) b \
IN_PROC_BROWSER_TEST_F(c, n##222) b \
IN_PROC_BROWSER_TEST_F(c, n##223) b \
IN_PROC_BROWSER_TEST_F(c, n##224) b \
IN_PROC_BROWSER_TEST_F(c, n##225) b \
IN_PROC_BROWSER_TEST_F(c, n##226) b \
IN_PROC_BROWSER_TEST_F(c, n##227) b \
IN_PROC_BROWSER_TEST_F(c, n##228) b \
IN_PROC_BROWSER_TEST_F(c, n##229) b \
IN_PROC_BROWSER_TEST_F(c, n##230) b \
IN_PROC_BROWSER_TEST_F(c, n##231) b \
IN_PROC_BROWSER_TEST_F(c, n##232) b \
IN_PROC_BROWSER_TEST_F(c, n##233) b \
IN_PROC_BROWSER_TEST_F(c, n##234) b \
IN_PROC_BROWSER_TEST_F(c, n##235) b \
IN_PROC_BROWSER_TEST_F(c, n##236) b \
IN_PROC_BROWSER_TEST_F(c, n##237) b \
IN_PROC_BROWSER_TEST_F(c, n##238) b \
IN_PROC_BROWSER_TEST_F(c, n##239) b \
IN_PROC_BROWSER_TEST_F(c, n##240) b \
IN_PROC_BROWSER_TEST_F(c, n##241) b \
IN_PROC_BROWSER_TEST_F(c, n##242) b \
IN_PROC_BROWSER_TEST_F(c, n##243) b \
IN_PROC_BROWSER_TEST_F(c, n##244) b \
IN_PROC_BROWSER_TEST_F(c, n##245) b \
IN_PROC_BROWSER_TEST_F(c, n##246) b \
IN_PROC_BROWSER_TEST_F(c, n##247) b \
IN_PROC_BROWSER_TEST_F(c, n##248) b \
IN_PROC_BROWSER_TEST_F(c, n##249) b \
IN_PROC_BROWSER_TEST_F(c, n##250) b \
IN_PROC_BROWSER_TEST_F(c, n##251) b \
IN_PROC_BROWSER_TEST_F(c, n##252) b \
IN_PROC_BROWSER_TEST_F(c, n##253) b \
IN_PROC_BROWSER_TEST_F(c, n##254) b \
IN_PROC_BROWSER_TEST_F(c, n##255) b \
IN_PROC_BROWSER_TEST_F(c, n##256) b \
IN_PROC_BROWSER_TEST_F(c, n##257) b \
IN_PROC_BROWSER_TEST_F(c, n##258) b \
IN_PROC_BROWSER_TEST_F(c, n##259) b \
IN_PROC_BROWSER_TEST_F(c, n##260) b \
IN_PROC_BROWSER_TEST_F(c, n##261) b \
IN_PROC_BROWSER_TEST_F(c, n##262) b \
IN_PROC_BROWSER_TEST_F(c, n##263) b \
IN_PROC_BROWSER_TEST_F(c, n##264) b \
IN_PROC_BROWSER_TEST_F(c, n##265) b \
IN_PROC_BROWSER_TEST_F(c, n##266) b \
IN_PROC_BROWSER_TEST_F(c, n##267) b \
IN_PROC_BROWSER_TEST_F(c, n##268) b \
IN_PROC_BROWSER_TEST_F(c, n##269) b \
IN_PROC_BROWSER_TEST_F(c, n##270) b \
IN_PROC_BROWSER_TEST_F(c, n##271) b \
IN_PROC_BROWSER_TEST_F(c, n##272) b \
IN_PROC_BROWSER_TEST_F(c, n##273) b \
IN_PROC_BROWSER_TEST_F(c, n##274) b \
IN_PROC_BROWSER_TEST_F(c, n##275) b \
IN_PROC_BROWSER_TEST_F(c, n##276) b \
IN_PROC_BROWSER_TEST_F(c, n##277) b \
IN_PROC_BROWSER_TEST_F(c, n##278) b \
IN_PROC_BROWSER_TEST_F(c, n##279) b \
IN_PROC_BROWSER_TEST_F(c, n##280) b \
IN_PROC_BROWSER_TEST_F(c, n##281) b \
IN_PROC_BROWSER_TEST_F(c, n##282) b \
IN_PROC_BROWSER_TEST_F(c, n##283) b \
IN_PROC_BROWSER_TEST_F(c, n##284) b \
IN_PROC_BROWSER_TEST_F(c, n##285) b \
IN_PROC_BROWSER_TEST_F(c, n##286) b \
IN_PROC_BROWSER_TEST_F(c, n##287) b \
IN_PROC_BROWSER_TEST_F(c, n##288) b \
IN_PROC_BROWSER_TEST_F(c, n##289) b \
IN_PROC_BROWSER_TEST_F(c, n##290) b \
IN_PROC_BROWSER_TEST_F(c, n##291) b \
IN_PROC_BROWSER_TEST_F(c, n##292) b \
IN_PROC_BROWSER_TEST_F(c, n##293) b \
IN_PROC_BROWSER_TEST_F(c, n##294) b \
IN_PROC_BROWSER_TEST_F(c, n##295) b \
IN_PROC_BROWSER_TEST_F(c, n##296) b \
IN_PROC_BROWSER_TEST_F(c, n##297) b \
IN_PROC_BROWSER_TEST_F(c, n##298) b \
IN_PROC_BROWSER_TEST_F(c, n##299) b

// Tests that worker ref count increments while extension API function is
// active.

// Flaky on Linux and ChromeOS, https://crbug.com/702126
#if defined(OS_LINUX)
#define MAYBE_WorkerRefCount DISABLED_WorkerRefCount
#else
#define MAYBE_WorkerRefCount WorkerRefCount
#endif
//IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, WorkerRefCount) {
//REPEAT_F(ServiceWorkerTest, WorkerRefCount, {
//  // Extensions APIs from SW are only enabled on trunk.
//  ScopedCurrentChannel current_channel_override(version_info::Channel::UNKNOWN);
//  const Extension* extension = LoadExtensionWithFlags(
//      test_data_dir_.AppendASCII("service_worker/api_worker_ref_count"),
//      kFlagNone);
//  ASSERT_TRUE(extension);
//  ui_test_utils::NavigateToURL(browser(),
//                               extension->GetResourceURL("page.html"));
//  content::WebContents* web_contents =
//      browser()->tab_strip_model()->GetActiveWebContents();
//
//  LOG(ERROR) << "1/X { WORKER STARTED";
//  ExtensionTestMessageListener worker_start_listener("WORKER STARTED", false);
//  worker_start_listener.set_failure_message("FAILURE");
//  ASSERT_TRUE(
//      content::ExecuteScript(web_contents, "window.runServiceWorker()"));
//  ASSERT_TRUE(worker_start_listener.WaitUntilSatisfied());
//  LOG(ERROR) << "2/X } WORKER STARTED";
//  LOG(ERROR) << "extension->url: " << extension->url().possibly_invalid_spec();
//
//  // Service worker should have no pending requests because it hasn't peformed
//  // any extension API request yet.
//  EXPECT_EQ(0u, GetWorkerRefCount(extension->url()));
//
//  LOG(ERROR) << "3/X { CHECK_REF_COUNT";
//  ExtensionTestMessageListener worker_listener("CHECK_REF_COUNT", true);
//  worker_listener.set_failure_message("FAILURE");
//  ASSERT_TRUE(content::ExecuteScript(web_contents, "window.testSendMessage()"));
//  ASSERT_TRUE(worker_listener.WaitUntilSatisfied());
//  LOG(ERROR) << "4/X } CHECK_REF_COUNT, sendMessage/1 in-flight";
//
//  // Service worker should have exactly one pending request because
//  // chrome.test.sendMessage() API call is in-flight.
//  EXPECT_EQ(1u, GetWorkerRefCount(extension->url()));
//
//  // Peform another extension API request while one is ongoing.
//  {
//    LOG(ERROR) << "5/X { CHECK_REF_COUNT/2";
//    ExtensionTestMessageListener listener("CHECK_REF_COUNT", true);
//    listener.set_failure_message("FAILURE");
//    ASSERT_TRUE(
//        content::ExecuteScript(web_contents, "window.testSendMessage()"));
//    ASSERT_TRUE(listener.WaitUntilSatisfied());
//    LOG(ERROR) << "6/X } CHECK_REF_COUNT/2, sendMessage/2 in-flight";
//
//    // Service worker currently has two extension API requests in-flight.
//    EXPECT_EQ(2u, GetWorkerRefCount(extension->url()));
//    // Finish executing the nested chrome.test.sendMessage() first.
//    listener.Reply("Hello world");
//    LOG(ERROR) << "7/X sendMessage/2 replied";
//  }
//
//  ExtensionTestMessageListener extension_listener("SUCCESS", false);
//  extension_listener.set_failure_message("FAILURE");
//  // Finish executing chrome.test.sendMessage().
//  worker_listener.Reply("Hello world");
//  LOG(ERROR) << "8/X sendMessage/1 replied";
//  ASSERT_TRUE(extension_listener.WaitUntilSatisfied());
//  LOG(ERROR) << "9/X SUCCESS WaitUntilSatisfied";
//
//  // The ref count should drop to 0.
//  EXPECT_EQ(0u, GetWorkerRefCount(extension->url()));
////}
//})

REPEAT_F(ServiceWorkerTest, WorkerRefCountCopy, {
  //TestBrowserClientForServiceWorker browser_client;
  //content::ContentBrowserClient* old_browser_client =
  //    content::SetBrowserClientForTesting(&browser_client);
  ScopedBrowserClientOverride client_override;

  // Extensions APIs from SW are only enabled on trunk.
  ScopedCurrentChannel current_channel_override(version_info::Channel::UNKNOWN);
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII("service_worker/api_worker_ref_count"),
      kFlagNone);
  ASSERT_TRUE(extension);
  ui_test_utils::NavigateToURL(browser(),
                               extension->GetResourceURL("page.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  LOG(ERROR) << "1/X { WORKER STARTED";
  ExtensionTestMessageListener worker_start_listener("WORKER STARTED", false);
  worker_start_listener.set_failure_message("FAILURE");
  ASSERT_TRUE(
      content::ExecuteScript(web_contents, "window.runServiceWorker()"));
  ASSERT_TRUE(worker_start_listener.WaitUntilSatisfied());
  LOG(ERROR) << "2/X } WORKER STARTED";
  LOG(ERROR) << "extension->url: " << extension->url().possibly_invalid_spec();

  // Service worker should have no pending requests because it hasn't peformed
  // any extension API request yet.
  EXPECT_EQ(0u, GetWorkerRefCount(extension->url()));

  LOG(ERROR) << "3/X { CHECK_REF_COUNT";
  ExtensionTestMessageListener worker_listener("CHECK_REF_COUNT", true);
  worker_listener.set_failure_message("FAILURE");
  ASSERT_TRUE(content::ExecuteScript(web_contents, "window.testSendMessage()"));
  ASSERT_TRUE(worker_listener.WaitUntilSatisfied());
  LOG(ERROR) << "4/X } CHECK_REF_COUNT, sendMessage/1 in-flight";

  // Service worker should have exactly one pending request because
  // chrome.test.sendMessage() API call is in-flight.
//  EXPECT_EQ(1u, GetWorkerRefCount(extension->url()));
//  ASSERT_TRUE(worker_listener.WaitUntilSatisfied());

  // Peform another extension API request while one is ongoing.
  {
    LOG(ERROR) << "5/X { CHECK_REF_COUNT/2";
    ExtensionTestMessageListener listener("CHECK_REF_COUNT", true);
    listener.set_failure_message("FAILURE");
    ASSERT_TRUE(
        content::ExecuteScript(web_contents, "window.testSendMessage()"));
    ASSERT_TRUE(listener.WaitUntilSatisfied());
    LOG(ERROR) << "6/X } CHECK_REF_COUNT/2, sendMessage/2 in-flight";

    // Service worker currently has two extension API requests in-flight.
    EXPECT_EQ(2u, GetWorkerRefCount(extension->url()));
    // Finish executing the nested chrome.test.sendMessage() first.
    listener.Reply("Hello world");
    LOG(ERROR) << "7/X sendMessage/2 replied";
  }

  client_override.WaitForMessageCount(1);
  content::RunAllPendingInMessageLoop(content::BrowserThread::IO);
  EXPECT_EQ(1u, GetWorkerRefCount(extension->url()));
//  ASSERT_TRUE(worker_listener.WaitUntilSatisfied());

  ExtensionTestMessageListener extension_listener("SUCCESS", false);
  extension_listener.set_failure_message("FAILURE");
  // Finish executing chrome.test.sendMessage().
  worker_listener.Reply("Hello world");
  LOG(ERROR) << "8/X sendMessage/1 replied";
  ASSERT_TRUE(extension_listener.WaitUntilSatisfied());
  LOG(ERROR) << "9/X SUCCESS WaitUntilSatisfied";

  // The ref count should drop to 0.
  //EXPECT_EQ(0u, GetWorkerRefCount(extension->url()));
  client_override.WaitForMessageCount(2);
  content::RunAllPendingInMessageLoop(content::BrowserThread::IO);
  EXPECT_EQ(0u, GetWorkerRefCount(extension->url()));
})

// This test loads a web page that has an iframe pointing to a
// chrome-extension:// URL. The URL is listed in the extension's
// web_accessible_resources. Initially the iframe is served from the extension's
// resource file. After verifying that, we register a Service Worker that
// controls the extension. Further requests to the same resource as before
// should now be served by the Service Worker.
// This test also verifies that if the requested resource exists in the manifest
// but is not present in the extension directory, the Service Worker can still
// serve the resource file.
IN_PROC_BROWSER_TEST_F(ServiceWorkerTest, WebAccessibleResourcesIframeSrc) {
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII(
          "service_worker/web_accessible_resources/iframe_src"),
      kFlagNone);
  ASSERT_TRUE(extension);
  ASSERT_TRUE(StartEmbeddedTestServer());

  // Service workers can only control secure contexts
  // (https://w3c.github.io/webappsec-secure-contexts/). For documents, this
  // typically means the document must have a secure origin AND all its ancestor
  // frames must have documents with secure origins.  However, extension pages
  // are considered secure, even if they have an ancestor document that is an
  // insecure context (see GetSchemesBypassingSecureContextCheckWhitelist). So
  // extension service workers must be able to control an extension page
  // embedded in an insecure context. To test this, set up an insecure
  // (non-localhost, non-https) URL for the web page. This page will create
  // iframes that load extension pages that must be controllable by service
  // worker.
  GURL page_url =
      embedded_test_server()->GetURL("a.com",
                                     "/extensions/api_test/service_worker/"
                                     "web_accessible_resources/webpage.html");
  EXPECT_FALSE(content::IsOriginSecure(page_url));

  content::WebContents* web_contents = AddTab(browser(), page_url);
  std::string result;
  // webpage.html will create an iframe pointing to a resource from |extension|.
  // Expect the resource to be served by the extension.
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, base::StringPrintf("window.testIframe('%s', 'iframe.html')",
                                       extension->id().c_str()),
      &result));
  EXPECT_EQ("FROM_EXTENSION_RESOURCE", result);

  ExtensionTestMessageListener service_worker_ready_listener("SW_READY", false);
  EXPECT_TRUE(ExecuteScriptInBackgroundPageNoWait(
      extension->id(), "window.registerServiceWorker()"));
  EXPECT_TRUE(service_worker_ready_listener.WaitUntilSatisfied());

  result.clear();
  // webpage.html will create another iframe pointing to a resource from
  // |extension| as before. But this time, the resource should be be served
  // from the Service Worker.
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, base::StringPrintf("window.testIframe('%s', 'iframe.html')",
                                       extension->id().c_str()),
      &result));
  EXPECT_EQ("FROM_SW_RESOURCE", result);

  result.clear();
  // webpage.html will create yet another iframe pointing to a resource that
  // exists in the extension manifest's web_accessible_resources, but is not
  // present in the extension directory. Expect the resources of the iframe to
  // be served by the Service Worker.
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      base::StringPrintf("window.testIframe('%s', 'iframe_non_existent.html')",
                         extension->id().c_str()),
      &result));
  EXPECT_EQ("FROM_SW_RESOURCE", result);
}

// Test is flaky. See https://crbug.com/737260
IN_PROC_BROWSER_TEST_F(ServiceWorkerBackgroundSyncTest, DISABLED_Sync) {
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII("service_worker/sync"), kFlagNone);
  ASSERT_TRUE(extension);
  ui_test_utils::NavigateToURL(browser(),
                               extension->GetResourceURL("page.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Prevent firing by going offline.
  content::background_sync_test_util::SetOnline(web_contents, false);

  ExtensionTestMessageListener sync_listener("SYNC: send-chats", false);
  sync_listener.set_failure_message("FAIL");

  std::string result;
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, "window.runServiceWorker()", &result));
  ASSERT_EQ("SERVICE_WORKER_READY", result);

  EXPECT_FALSE(sync_listener.was_satisfied());
  // Resume firing by going online.
  content::background_sync_test_util::SetOnline(web_contents, true);
  EXPECT_TRUE(sync_listener.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerTest,
                       FetchFromContentScriptShouldNotGoToServiceWorkerOfPage) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  GURL page_url = embedded_test_server()->GetURL(
      "/extensions/api_test/service_worker/content_script_fetch/"
      "controlled_page/index.html");
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), page_url);
  content::WaitForLoadStop(tab);

  std::string value;
  ASSERT_TRUE(
      content::ExecuteScriptAndExtractString(tab, "register();", &value));
  EXPECT_EQ("SW controlled", value);

  ASSERT_TRUE(RunExtensionTest("service_worker/content_script_fetch"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(ServiceWorkerPushMessagingTest, OnPush) {
  const Extension* extension = LoadExtensionWithFlags(
      test_data_dir_.AppendASCII("service_worker/push_messaging"), kFlagNone);
  ASSERT_TRUE(extension);
  GURL extension_url = extension->url();

  ASSERT_NO_FATAL_FAILURE(GrantNotificationPermissionForTest(extension_url));

  GURL url = extension->GetResourceURL("page.html");
  ui_test_utils::NavigateToURL(browser(), url);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Start the ServiceWorker.
  ExtensionTestMessageListener ready_listener("SERVICE_WORKER_READY", false);
  ready_listener.set_failure_message("SERVICE_WORKER_FAILURE");
  const char* kScript = "window.runServiceWorker()";
  EXPECT_TRUE(content::ExecuteScript(web_contents->GetMainFrame(), kScript));
  EXPECT_TRUE(ready_listener.WaitUntilSatisfied());

  PushMessagingAppIdentifier app_identifier =
      GetAppIdentifierForServiceWorkerRegistration(0LL, extension_url);
  ASSERT_EQ(app_identifier.app_id(), gcm_driver()->last_gettoken_app_id());
  EXPECT_EQ("1234567890", gcm_driver()->last_gettoken_authorized_entity());

  base::RunLoop run_loop;
  // Send a push message via gcm and expect the ServiceWorker to receive it.
  ExtensionTestMessageListener push_message_listener("OK", false);
  push_message_listener.set_failure_message("FAIL");
  gcm::IncomingMessage message;
  message.sender_id = "1234567890";
  message.raw_data = "testdata";
  message.decrypted = true;
  push_service()->SetMessageCallbackForTesting(run_loop.QuitClosure());
  push_service()->OnMessage(app_identifier.app_id(), message);
  EXPECT_TRUE(push_message_listener.WaitUntilSatisfied());
  run_loop.Run();  // Wait until the message is handled by push service.
}

}  // namespace extensions
