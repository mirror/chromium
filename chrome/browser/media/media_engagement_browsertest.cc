// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/media/media_engagement_contents_observer.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.cc"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {

const char* kMediaEngagementFeatureFlag = "media-engagement";

const char* kMediaEngagementTestDataPath = "chrome/test/data/media/engagement";

const base::string16 kReadyTitle = base::ASCIIToUTF16("Ready");

}  // namespace

// Class used to test the Media Engagement service.
class MediaEngagementBrowserTest : public InProcessBrowserTest {
 public:
  MediaEngagementBrowserTest() : timer_(new base::Timer(true, false)) {
    http_server_.ServeFilesFromSourceDirectory(kMediaEngagementTestDataPath);
    http_server_origin2_.ServeFilesFromSourceDirectory(
        kMediaEngagementTestDataPath);
  }

  ~MediaEngagementBrowserTest() override{};

  void SetUp() override {
    ASSERT_TRUE(http_server_.Start());
    ASSERT_TRUE(http_server_origin2_.Start());

    InProcessBrowserTest::SetUp();
  };

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII("enable-features",
                                    kMediaEngagementFeatureFlag);
  };

  void LoadTestPage(const std::string& page) {
    ui_test_utils::NavigateToURL(browser(), http_server_.GetURL("/" + page));
  };

  void LoadTestPageAndWait(const std::string& page) {
    LoadTestPage(page);
    WaitForTitle();
  };

  void WaitFor(base::TimeDelta time) {
    timer_->Start(FROM_HERE, time,
                  base::Bind(&MediaEngagementBrowserTest::TimerFinished,
                             base::Unretained(this)));
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  void WaitForMax() { WaitFor(MediaEngagementBrowserTest::kMaxWaitingTime); }

  void ExpectScores(int visits, int media_playbacks) {
    ExpectScores(http_server_.GetURL("/"), visits, media_playbacks);
  }

  void ExpectScoresSecondOrigin(int visits, int media_playbacks) {
    ExpectScores(http_server_origin2_.GetURL("/"), visits, media_playbacks);
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ExecuteScript(const std::string& script) {
    EXPECT_TRUE(content::ExecuteScript(web_contents(), script));
  }

  void OpenTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), GURL("chrome://about"),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  }

  void LoadSubFrame(const std::string& page) {
    ExecuteScript("window.open(\"" +
                  http_server_origin2_.GetURL("/" + page).spec() +
                  "\", \"subframe\")");
  }

  void WaitForTitle() {
    content::TitleWatcher title_watcher(web_contents(), kReadyTitle);
    EXPECT_EQ(kReadyTitle, title_watcher.WaitAndGetTitle());
  }

 private:
  void ExpectScores(GURL url, int visits, int media_playbacks) {
    MediaEngagementService* service =
        MediaEngagementService::Get(browser()->profile());
    MediaEngagementScore* score = service->CreateEngagementScore(url);
    EXPECT_EQ(visits, score->visits());
    EXPECT_EQ(media_playbacks, score->media_playbacks());
    delete score;
  }

  void TimerFinished() { run_loop_->Quit(); };

  net::EmbeddedTestServer http_server_;
  net::EmbeddedTestServer http_server_origin2_;

  std::unique_ptr<base::Timer> timer_;
  std::unique_ptr<base::RunLoop> run_loop_;

  const base::TimeDelta kMaxWaitingTime =
      MediaEngagementContentsObserver::kSignificantMediaPlaybackTime +
      base::TimeDelta::FromSeconds(2);
};

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest, RecordEngagement) {
  LoadTestPageAndWait("engagement_test.html");
  WaitForMax();
  ExpectScores(1, 1);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NotTime) {
  LoadTestPageAndWait("engagement_test.html");
  WaitFor(base::TimeDelta::FromSeconds(1));
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_TabMuted) {
  web_contents()->SetAudioMuted(true);
  LoadTestPageAndWait("engagement_test.html");
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_PlayerMuted) {
  LoadTestPageAndWait("engagement_test_muted.html");
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_PlaybackStopped) {
  LoadTestPageAndWait("engagement_test.html");
  WaitFor(base::TimeDelta::FromSeconds(1));
  ExecuteScript("document.getElementById(\"video\").pause();");
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NotVisible) {
  LoadTestPageAndWait("engagement_test.html");
  OpenTab();
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_FrameSize) {
  LoadTestPageAndWait("engagement_test_small_frame_size.html");
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NoAudioTrack) {
  LoadTestPageAndWait("engagement_test_no_audio_track.html");
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_SilentAudioTrack) {
  LoadTestPageAndWait("engagement_test_silent_audio_track.html");
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest, IFrameDelegation) {
  LoadTestPage("engagement_test_iframe.html");
  LoadSubFrame("engagement_test_iframe_child.html");

  WaitForTitle();
  WaitForMax();

  ExpectScores(1, 1);
  ExpectScoresSecondOrigin(0, 0);
}
