// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
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

static const char* kMediaEngagementFeatureFlag = "media-engagement";

static const char* kMediaEngagementTestDataPath =
    "chrome/test/data/media/engagement";

static const char kMediaEngagementGetPlayerJS[] =
    "var v = document.getElementById(\"video\");"
    " window.domAutomationController.send(v.currentTime > 0"
    " && !v.paused && v.readyState >= 2);";

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

  void WaitFor(base::TimeDelta time) {
    timer_->Start(FROM_HERE, time,
                  base::Bind(&MediaEngagementBrowserTest::TimerFinished,
                             base::Unretained(this)));
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  void WaitForMax() {
    WaitFor(MediaEngagementContentsObserver::kSignificantMediaPlaybackTime);
  }

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

  bool GetPlayerPlayingState(const content::ToRenderFrameHost& adapter) {
    bool playing = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        adapter, kMediaEngagementGetPlayerJS, &playing));
    return playing;
  }

  void CheckPlayerIsPlaying() {
    EXPECT_TRUE(GetPlayerPlayingState(web_contents()));
  }

  void CheckPlayerIsPlayingOnFrame() {
    content::RenderFrameHost* child_rfh = web_contents()->GetAllFrames()[1];
    EXPECT_TRUE(GetPlayerPlayingState(child_rfh));
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
};

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest, RecordEngagement) {
  LoadTestPage("engagement_test.html");
  WaitForMax();
  CheckPlayerIsPlaying();
  ExpectScores(1, 1);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NotTime) {
  LoadTestPage("engagement_test.html");
  WaitFor(base::TimeDelta::FromSeconds(1));
  CheckPlayerIsPlaying();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_TabMuted) {
  web_contents()->SetAudioMuted(true);
  LoadTestPage("engagement_test.html");
  WaitForMax();
  CheckPlayerIsPlaying();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_PlayerMuted) {
  LoadTestPage("engagement_test_muted.html");
  WaitForMax();
  CheckPlayerIsPlaying();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_PlaybackStopped) {
  LoadTestPage("engagement_test.html");
  WaitFor(base::TimeDelta::FromSeconds(1));
  ExecuteScript("document.getElementById(\"video\").pause();");
  WaitForMax();
  EXPECT_FALSE(GetPlayerPlayingState(web_contents()));
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NotVisible) {
  LoadTestPage("engagement_test.html");
  OpenTab();
  WaitForMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_FrameSize) {
  LoadTestPage("engagement_test_small_frame_size.html");
  WaitForMax();
  CheckPlayerIsPlaying();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NoAudioTrack) {
  LoadTestPage("engagement_test_no_audio_track.html");
  WaitForMax();
  CheckPlayerIsPlaying();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_SilentAudioTrack) {
  LoadTestPage("engagement_test_silent_audio_track.html");
  WaitForMax();
  CheckPlayerIsPlaying();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest, IFrameDelegation) {
  LoadTestPage("engagement_test_iframe.html");
  LoadSubFrame("engagement_test_iframe_child.html");

  WaitFor(base::TimeDelta::FromSeconds(2));
  WaitForMax();
  CheckPlayerIsPlayingOnFrame();

  ExpectScores(1, 1);
  ExpectScoresSecondOrigin(0, 0);
}
