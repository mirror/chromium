// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_data.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_fetcher.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_service.h"
#include "chrome/browser/search/one_google_bar/one_google_bar_service_factory.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/search/instant_test_base.h"
#include "chrome/browser/ui/search/instant_test_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

content::WebContents* OpenNewTab(Browser* browser, const GURL& url) {
  ui_test_utils::NavigateToURLWithDisposition(
      browser, url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  return browser->tab_strip_model()->GetActiveWebContents();
}

}  // namespace

// A test class that sets up voice_text_browsertest.html (which is mostly a copy
// of the text module part from the real local_ntp.html) as the NTP URL.
class LocalNTPVoiceTextTest : public InProcessBrowserTest,
                              public InstantTestBase {
 public:
  LocalNTPVoiceTextTest() {}

 protected:
  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(https_test_server().Start());
    GURL instant_url =
        https_test_server().GetURL("/instant_extended.html?strk=1&");
    GURL ntp_url = https_test_server().GetURL(
        "/local_ntp/voice_text_browsertest.html?strk=1&");
    InstantTestBase::Init(instant_url, ntp_url, false);
  }
};

// This runs a bunch of pure JS-side tests, i.e. those that don't require any
// interaction from the native side.
IN_PROC_BROWSER_TEST_F(LocalNTPVoiceTextTest, SimpleJavascriptTests) {
  if (content::AreAllSitesIsolatedForTesting()) {
    LOG(ERROR) << "LocalNTPVoiceTextTest.SimpleJavascriptTests doesn't work in "
                  "--site-per-process mode yet, see crbug.com/695221.";
    return;
  }

  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));

  content::WebContents* active_tab = OpenNewTab(browser(), ntp_url());
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  bool success = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!runSimpleTests('voice-text-template')", &success));
  EXPECT_TRUE(success);
}

// A test class that sets up voice_microphone_browsertest.html
// (which is mostly a copy of the microphone module part
// from the real local_ntp.html) as the NTP URL.
class LocalNTPVoiceMicrophoneTest : public InProcessBrowserTest,
                                    public InstantTestBase {
 public:
  LocalNTPVoiceMicrophoneTest() {}

 protected:
  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(https_test_server().Start());
    GURL instant_url =
        https_test_server().GetURL("/instant_extended.html?strk=1&");
    GURL ntp_url = https_test_server().GetURL(
        "/local_ntp/voice_microphone_browsertest.html?strk=1&");
    InstantTestBase::Init(instant_url, ntp_url, false);
  }
};

// This runs a bunch of pure JS-side tests, i.e. those that don't require any
// interaction from the native side.
IN_PROC_BROWSER_TEST_F(LocalNTPVoiceMicrophoneTest, SimpleJavascriptTests) {
  if (content::AreAllSitesIsolatedForTesting()) {
    LOG(ERROR) << "LocalNTPVoiceMicrophoneTest.SimpleJavascriptTests doesn't"
                  " work in --site-per-process mode yet, see crbug.com/695221.";
    return;
  }

  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));

  content::WebContents* active_tab = OpenNewTab(browser(), ntp_url());
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  bool success = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!runSimpleTests('voice-microphone-template')", &success));
  EXPECT_TRUE(success);
}

// A test class that sets up voice_view_browsertest.html (which is mostly a copy
// of the view module part from the real local_ntp.html) as the NTP URL.
class LocalNTPVoiceViewTest : public InProcessBrowserTest,
                              public InstantTestBase {
 public:
  LocalNTPVoiceViewTest() {}

 protected:
  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(https_test_server().Start());
    GURL instant_url =
        https_test_server().GetURL("/instant_extended.html?strk=1&");
    GURL ntp_url = https_test_server().GetURL(
        "/local_ntp/voice_view_browsertest.html?strk=1&");
    InstantTestBase::Init(instant_url, ntp_url, false);
  }
};

// This runs a bunch of pure JS-side tests, i.e. those that don't require any
// interaction from the native side.
IN_PROC_BROWSER_TEST_F(LocalNTPVoiceViewTest, SimpleJavascriptTests) {
  if (content::AreAllSitesIsolatedForTesting()) {
    LOG(ERROR) << "LocalNTPVoiceViewTest.SimpleJavascriptTests doesn't work in "
                  "--site-per-process mode yet, see crbug.com/695221.";
    return;
  }

  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));

  content::WebContents* active_tab = OpenNewTab(browser(), ntp_url());
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  bool success = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!runSimpleTests('voice-view-template')", &success));
  EXPECT_TRUE(success);
}

// A test class that sets up voice_speech_browsertest.html (which is mostly
// a copy of the view module part from the real local_ntp.html) as the NTP URL.
class LocalNTPVoiceSpeechTest : public InProcessBrowserTest,
                                public InstantTestBase {
 public:
  LocalNTPVoiceSpeechTest() {}

 protected:
  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(https_test_server().Start());
    GURL instant_url =
        https_test_server().GetURL("/instant_extended.html?strk=1&");
    GURL ntp_url = https_test_server().GetURL(
        "/local_ntp/voice_speech_browsertest.html?strk=1&");
    InstantTestBase::Init(instant_url, ntp_url, false);
  }
};

// This runs a bunch of pure JS-side tests, i.e. those that don't require any
// interaction from the native side.
IN_PROC_BROWSER_TEST_F(LocalNTPVoiceSpeechTest, SimpleJavascriptTests) {
  if (content::AreAllSitesIsolatedForTesting()) {
    LOG(ERROR) << "LocalNTPVoiceSpeechTest.SimpleJavascriptTests doesn't"
                  " work in --site-per-process mode yet, see crbug.com/695221.";
    return;
  }

  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));

  content::WebContents* active_tab = OpenNewTab(browser(), ntp_url());
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  bool success = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!runSimpleTests('voice-speech-template')", &success));
  EXPECT_TRUE(success);
}
