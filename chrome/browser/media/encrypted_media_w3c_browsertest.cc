// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "media/base/media_switches.h"
#include "media/base/test_data_util.cc"
#include "media/media_features.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

// The W3C EME tests use MP4 files, which aren't supported by content_shell.
// As a result, very few of the W3C tests can be run as layout tests. This
// runs some of the W3C tests as browser_tests, where MP4 files may be
// supported.

#if !BUILDFLAG(USE_PROPRIETARY_CODECS)
static_assert(false, "This should only be compiled if MP4 is supported.");
#endif

// Note that 1/2 of the W3C tests run with a DRM system (Widevine for Chrome).
// However, the tests communicate with an external license server, so they can
// not be run as browser_tests.

// Runner used to run the W3C tests. This is a wrapper that simply waits for
// the test to complete and then reports the result using the page title.
const char kW3CTestRunner[] = "/eme_w3c_test_runner.html";

// Parameter used to specify the W3C test to run.
const char kTestToRun[] = "test";

// Wrapper specific test results and errors. These must match the values used
// in |kW3CTestRunner|.
const char kSuccess[] = "success";
const char kFailure[] = "failure";

// Location of the W3C tests.
const base::FilePath::CharType kW3CTestDirectory[] =
    FILE_PATH_LITERAL("third_party/WebKit/LayoutTests/external/wpt");
const char kEncryptedMediaDirectory[] = "/encrypted-media/";

class EncryptedMediaW3CTest : public InProcessBrowserTest,
                              public content::WebContentsObserver {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kAutoplayPolicy,
        switches::autoplay::kNoUserGestureRequiredPolicy);
  }

  void TestW3CPlayback(const std::string& w3c_test_page) {
    // W3C tests run using absolute paths, so start a web server in the
    // correct directory.
    DVLOG(0) << base::TimeFormatTimeOfDayWithMilliseconds(base::Time::Now())
             << " Starting HTTPS server for W3C files";
    auto w3c_test_server = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    w3c_test_server->ServeFilesFromSourceDirectory(
        base::FilePath(kW3CTestDirectory));
    CHECK(w3c_test_server->Start());
    GURL w3c_gurl =
        w3c_test_server->GetURL(kEncryptedMediaDirectory + w3c_test_page);

    // The wrapper is in a seperate directory as the W3C tests are cloned
    // from GitHub.
    DVLOG(0) << base::TimeFormatTimeOfDayWithMilliseconds(base::Time::Now())
             << " Starting HTTPS server for test file";
    auto https_test_server = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_test_server->ServeFilesFromSourceDirectory(media::GetTestDataPath());
    CHECK(https_test_server->Start());

    // Now generate the web page URL for the wrapper.
    GURL gurl = https_test_server->GetURL(std::string(kW3CTestRunner) + "?" +
                                          kTestToRun + "=" + w3c_gurl.spec());

    // Page title is used to indicate the result of the test, so watch for
    // title changes.
    Observe(browser()->tab_strip_model()->GetActiveWebContents());
    content::TitleWatcher title_watcher(
        browser()->tab_strip_model()->GetActiveWebContents(),
        base::ASCIIToUTF16(kSuccess));
    title_watcher.AlsoWaitForTitle(base::ASCIIToUTF16(kFailure));

    // Load the test page and see what happens.
    DVLOG(0) << base::TimeFormatTimeOfDayWithMilliseconds(base::Time::Now())
             << " Running test URL: " << gurl;
    ui_test_utils::NavigateToURL(browser(), gurl);
    std::string result = base::UTF16ToASCII(title_watcher.WaitAndGetTitle());
    EXPECT_EQ(kSuccess, result);
  }
};

// Most of the W3C tests play encrypted content for a couple of seconds, so
// only including a small number of tests to reduce the total time taken.

IN_PROC_BROWSER_TEST_F(
    EncryptedMediaW3CTest,
    ClearKey_MP4_Playback_Temporary_Encrypted_Clear_Sources) {
  TestW3CPlayback(
      "clearkey-mp4-playback-temporary-encrypted-clear-sources.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Playback_Temporary_Events) {
  TestW3CPlayback("clearkey-mp4-playback-temporary-events.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest, ClearKey_MP4_Playback_Temporary) {
  TestW3CPlayback("clearkey-mp4-playback-temporary.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Playback_Temporary_Multikey) {
  TestW3CPlayback("clearkey-mp4-playback-temporary-multikey.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Playback_Temporary_Multisession) {
  TestW3CPlayback("clearkey-mp4-playback-temporary-multisession.https.html");
}

IN_PROC_BROWSER_TEST_F(
    EncryptedMediaW3CTest,
    ClearKey_MP4_Playback_Temporary_SetMediaKeys_After_Update) {
  TestW3CPlayback(
      "clearkey-mp4-playback-temporary-setMediaKeys-after-update.https.html");
}

IN_PROC_BROWSER_TEST_F(
    EncryptedMediaW3CTest,
    ClearKey_MP4_Playback_Temporary_SetMediaKeys_Immediately) {
  TestW3CPlayback(
      "clearkey-mp4-playback-temporary-setMediaKeys-immediately.https.html");
}

IN_PROC_BROWSER_TEST_F(
    EncryptedMediaW3CTest,
    ClearKey_MP4_Playback_Temporary_SetMediaKeys_Onencrypted) {
  TestW3CPlayback(
      "clearkey-mp4-playback-temporary-setMediaKeys-onencrypted.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Playback_Temporary_Two_Videos) {
  TestW3CPlayback("clearkey-mp4-playback-temporary-two-videos.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Playback_Temporary_WaitingForKey) {
  TestW3CPlayback("clearkey-mp4-playback-temporary-waitingforkey.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Reset_Src_After_SetMediaKeys) {
  TestW3CPlayback("clearkey-mp4-reset-src-after-setmediakeys.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_SetMediaKeys_Again_After_Playback) {
  TestW3CPlayback("clearkey-mp4-setmediakeys-again-after-playback.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest, ClearKey_MP4_SetMediaKeys) {
  TestW3CPlayback("clearkey-mp4-setmediakeys.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_SetMediaKeys_At_Same_Time) {
  TestW3CPlayback("clearkey-mp4-setmediakeys-at-same-time.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_SetMediaKeys_To_Multiple_Video_Elements) {
  TestW3CPlayback(
      "clearkey-mp4-setmediakeys-to-multiple-video-elements.https.html");
}

IN_PROC_BROWSER_TEST_F(
    EncryptedMediaW3CTest,
    ClearKey_MP4_SetMediaKeys_Multiple_Times_With_Different_MediaKeys) {
  TestW3CPlayback(
      "clearkey-mp4-setmediakeys-multiple-times-with-different-mediakeys.https."
      "html");
}

IN_PROC_BROWSER_TEST_F(
    EncryptedMediaW3CTest,
    ClearKey_MP4_SetMediaKeys_Multiple_Times_With_The_Same_MediaKeys) {
  TestW3CPlayback(
      "clearkey-mp4-setmediakeys-multiple-times-with-the-same-mediakeys.https."
      "html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Syntax_MediaKeySession) {
  TestW3CPlayback("clearkey-mp4-syntax-mediakeysession.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest, ClearKey_MP4_Syntax_MediaKeys) {
  TestW3CPlayback("clearkey-mp4-syntax-mediakeys.https.html");
}

IN_PROC_BROWSER_TEST_F(EncryptedMediaW3CTest,
                       ClearKey_MP4_Syntax_MediaKeySystemAccess) {
  TestW3CPlayback("clearkey-mp4-syntax-mediakeysystemaccess.https.html");
}
