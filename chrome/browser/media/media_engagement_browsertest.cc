// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/media/media_engagement_contents_observer.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test_utils.cc"
#include "media/base/media_switches.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {

const char* kMediaEngagementTestDataPath = "chrome/test/data/media/engagement";

const base::string16 kReadyTitle = base::ASCIIToUTF16("Ready");

// Watches WasRecentlyAudible changes on a WebContents, blocking until the
// tab is audible.
class WasRecentlyAudibleWatcher {
 public:
  // |web_contents| must be non-NULL and needs to stay alive for the
  // entire lifetime of |this|.
  explicit WasRecentlyAudibleWatcher(content::WebContents* web_contents)
      : web_contents_(web_contents), timer_(new base::Timer(true, true)){};
  ~WasRecentlyAudibleWatcher() = default;

  // Waits until WasRecentlyAudible is true.
  void AdvanceWasRecentlyAudible() {
    if (!web_contents_->WasRecentlyAudible()) {
      timer_->Start(
          FROM_HERE, base::TimeDelta::FromMicroseconds(100),
          base::Bind(&WasRecentlyAudibleWatcher::TestWasRecentlyAudible,
                     base::Unretained(this)));
      run_loop_.reset(new base::RunLoop());
      run_loop_->Run();
    }
  };

 private:
  void TestWasRecentlyAudible() {
    if (web_contents_->WasRecentlyAudible()) {
      run_loop_->Quit();
      timer_->Stop();
    }
  };

  content::WebContents* web_contents_;

  std::unique_ptr<base::Timer> timer_;
  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(WasRecentlyAudibleWatcher);
};

}  // namespace

// Class used to test the Media Engagement service.
class MediaEngagementBrowserTest : public InProcessBrowserTest {
 public:
  MediaEngagementBrowserTest()
      : task_runner_(new base::TestMockTimeTaskRunner()) {
    http_server_.ServeFilesFromSourceDirectory(kMediaEngagementTestDataPath);
    http_server_origin2_.ServeFilesFromSourceDirectory(
        kMediaEngagementTestDataPath);
  }

  ~MediaEngagementBrowserTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(http_server_.Start());
    ASSERT_TRUE(http_server_origin2_.Start());

    scoped_feature_list_.InitAndEnableFeature(media::kMediaEngagement);

    InProcessBrowserTest::SetUp();
  };

  void LoadTestPage(const std::string& page) {
    // We can't do this in SetUp as the browser isn't ready yet and we
    // need it before the page navigates.
    InjectTimerTaskRunner();

    ui_test_utils::NavigateToURL(browser(), http_server_.GetURL("/" + page));
  };

  void LoadTestPageAndAdvancePlay(const std::string& page) {
    LoadTestPage(page);
    AdvancePlay();
  }

  void LoadTestPageAndAdvancePlayAndAudible(const std::string& page) {
    LoadTestPageAndAdvancePlay(page);
    AdvanceWasRecentlyAudible();
  };

  void Advance(base::TimeDelta time) {
    task_runner_->FastForwardBy(time);
    base::RunLoop().RunUntilIdle();
  }

  void AdvanceMax() { Advance(MediaEngagementBrowserTest::kMaxWaitingTime); }

  void ExpectScores(int visits, int media_playbacks) {
    ExpectScores(http_server_.GetURL("/"), visits, media_playbacks);
  }

  void ExpectScoresSecondOrigin(int visits, int media_playbacks) {
    ExpectScores(http_server_origin2_.GetURL("/"), visits, media_playbacks);
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ExecuteScript(const std::string& script) {
    EXPECT_TRUE(content::ExecuteScript(GetWebContents(), script));
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

  void AdvancePlay() {
    content::TitleWatcher title_watcher(GetWebContents(), kReadyTitle);
    EXPECT_EQ(kReadyTitle, title_watcher.WaitAndGetTitle());
  }

  void AdvanceWasRecentlyAudible() {
    WasRecentlyAudibleWatcher watcher(GetWebContents());
    watcher.AdvanceWasRecentlyAudible();
  }

 private:
  void ExpectScores(GURL url, int visits, int media_playbacks) {
    MediaEngagementScore* score = GetService()->CreateEngagementScore(url);
    EXPECT_EQ(visits, score->visits());
    EXPECT_EQ(media_playbacks, score->media_playbacks());
    delete score;
  }

  void InjectTimerTaskRunner() {
    contents_observer()->playback_timer_->SetTaskRunner(task_runner_);
  }

  MediaEngagementService* GetService() {
    return MediaEngagementService::Get(browser()->profile());
  }

  MediaEngagementContentsObserver* contents_observer() {
    MediaEngagementService* mei_service = GetService();
    DCHECK(mei_service->contents_observers_.size() == 1);
    return *mei_service->contents_observers_.begin();
  }

  net::EmbeddedTestServer http_server_;
  net::EmbeddedTestServer http_server_origin2_;

  base::test::ScopedFeatureList scoped_feature_list_;

  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;

  const base::TimeDelta kMaxWaitingTime =
      MediaEngagementContentsObserver::kSignificantMediaPlaybackTime +
      base::TimeDelta::FromSeconds(2);
};

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest, RecordEngagement) {
  LoadTestPageAndAdvancePlayAndAudible("engagement_test.html");
  AdvanceMax();
  ExpectScores(1, 1);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NotTime) {
  LoadTestPageAndAdvancePlayAndAudible("engagement_test.html");
  Advance(base::TimeDelta::FromSeconds(1));
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_TabMuted) {
  GetWebContents()->SetAudioMuted(true);
  LoadTestPageAndAdvancePlayAndAudible("engagement_test.html");
  AdvanceMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_PlayerMuted) {
  LoadTestPageAndAdvancePlay("engagement_test_muted.html");
  AdvanceMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_PlaybackStopped) {
  LoadTestPageAndAdvancePlayAndAudible("engagement_test.html");
  Advance(base::TimeDelta::FromSeconds(1));
  ExecuteScript("document.getElementById(\"video\").pause();");
  AdvanceMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NotVisible) {
  LoadTestPageAndAdvancePlayAndAudible("engagement_test.html");
  OpenTab();
  AdvanceMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_FrameSize) {
  LoadTestPageAndAdvancePlayAndAudible("engagement_test_small_frame_size.html");
  AdvanceMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_NoAudioTrack) {
  LoadTestPageAndAdvancePlay("engagement_test_no_audio_track.html");
  AdvanceMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest,
                       DoNotRecordEngagement_SilentAudioTrack) {
  LoadTestPageAndAdvancePlay("engagement_test_silent_audio_track.html");
  AdvanceMax();
  ExpectScores(1, 0);
}

IN_PROC_BROWSER_TEST_F(MediaEngagementBrowserTest, IFrameDelegation) {
  LoadTestPage("engagement_test_iframe.html");
  LoadSubFrame("engagement_test_iframe_child.html");

  AdvancePlay();
  AdvanceWasRecentlyAudible();
  AdvanceMax();

  ExpectScores(1, 1);
  ExpectScoresSecondOrigin(0, 0);
}
