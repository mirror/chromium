// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_contents_observer.h"

#include "base/test/scoped_feature_list.h"
#include "base/timer/mock_timer.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "chrome/browser/media/media_engagement_service_factory.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

class MediaEngagementContentsObserverTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    scoped_feature_list_.InitFromCommandLine(
        "media-engagement,media-engagement-bypass-autoplay-policies",
        std::string());

    ChromeRenderViewHostTestHarness::SetUp();

    service_ = MediaEngagementService::Get(profile());
    ASSERT_TRUE(service_);
    contents_observer_ =
        new MediaEngagementContentsObserver(web_contents(), service_);

    playback_timer_ = new base::MockTimer(true, false);
    contents_observer_->SetTimerForTest(base::WrapUnique(playback_timer_));

    ASSERT_FALSE(GetStoredPlayerStatesCount());
  }

  bool IsTimerRunning() const { return playback_timer_->IsRunning(); }

  bool WasSignificantPlaybackRecorded() const {
    return contents_observer_->significant_playback_recorded_;
  }

  size_t GetSignificantActivePlayersCount() const {
    return contents_observer_->significant_players_.size();
  }

  size_t GetStoredPlayerStatesCount() const {
    return contents_observer_->player_states_.size();
  }

  void SimulatePlaybackStarted(int id) {
    content::WebContentsObserver::MediaPlayerInfo player_info(true, true);
    SimulatePlaybackStarted(player_info, id);
  }

  void SimulatePlaybackStarted(
      content::WebContentsObserver::MediaPlayerInfo player_info,
      int id) {
    content::WebContentsObserver::MediaPlayerId player_id =
        std::make_pair(nullptr /* RenderFrameHost */, id);
    contents_observer_->MediaStartedPlaying(player_info, player_id);
    SimulateMutedStateChange(id, false);
  }

  void SimulatePlaybackStopped(int id) {
    content::WebContentsObserver::MediaPlayerInfo player_info(true, true);
    content::WebContentsObserver::MediaPlayerId player_id =
        std::make_pair(nullptr /* RenderFrameHost */, id);
    contents_observer_->MediaStoppedPlaying(player_info, player_id);
  }

  void SimulateMutedStateChange(int id, bool muted_state) {
    content::WebContentsObserver::MediaPlayerId player_id =
        std::make_pair(nullptr /* RenderFrameHost */, id);
    contents_observer_->MediaMutedStateChanged(player_id, muted_state);
  }

  void SimulateIsVisible() { contents_observer_->WasShown(); }

  void SimulateIsHidden() { contents_observer_->WasHidden(); }

  bool AreConditionsMet() const {
    return contents_observer_->AreConditionsMet();
  }

  void SimulateSignificantPlaybackRecorded() {
    contents_observer_->significant_playback_recorded_ = true;
  }

  void SimulateSignificantPlaybackTime() {
    contents_observer_->OnSignificantMediaPlaybackTime();
  }

  void SimulatePlaybackTimerFired() { playback_timer_->Fire(); }

  void ExpectScores(GURL url,
                    int expected_visits,
                    int expected_media_playbacks) {
    double expected_score = MediaEngagementScore::CalculateScore(
        expected_visits, expected_media_playbacks);
    EXPECT_EQ(contents_observer_->service_->GetEngagementScore(url),
              expected_score);
    EXPECT_EQ(contents_observer_->service_->GetScoreMap()[url], expected_score);

    MediaEngagementScore score =
        contents_observer_->service_->CreateEngagementScore(url);
    EXPECT_EQ(score.visits(), expected_visits);
    EXPECT_EQ(score.media_playbacks(), expected_media_playbacks);
  }

  void ExpectAutoplayOriginScope(content::RenderFrameHost* rfh,
                                 std::string origin) {
    content::WebPreferences preferences =
        rfh->GetRenderViewHost()->GetWebkitPreferences();
    std::string scope = preferences.media_playback_gesture_whitelist_scope;
    if (scope.length())
      scope = scope + "/";
    EXPECT_EQ(origin, scope);
  }

  void SetMetricsForOrigin(GURL url, int visits, int media_playbacks) {
    MediaEngagementScore score = service_->CreateEngagementScore(url);
    score.SetVisits(visits);
    score.SetMediaPlaybacks(media_playbacks);
    score.Commit();
  }

  double GetEngagementScore(GURL url) {
    return service_->GetEngagementScore(url);
  }

  content::RenderFrameHost* CreateFrame(content::RenderFrameHost* parent_rfh,
                                        GURL url) {
    content::RenderFrameHost* child_rfh =
        content::RenderFrameHostTester::For(parent_rfh)
            ->AppendChild("subframe");
    content::RenderFrameHostTester* subframe_tester =
        content::RenderFrameHostTester::For(child_rfh);
    subframe_tester->SimulateNavigationStart(url);
    subframe_tester->SimulateNavigationCommit(url);
    subframe_tester->SimulateNavigationStop();
    return child_rfh;
  }

 private:
  // contents_observer_ auto-destroys when WebContents is destroyed.
  MediaEngagementContentsObserver* contents_observer_;

  MediaEngagementService* service_;

  base::test::ScopedFeatureList scoped_feature_list_;

  base::MockTimer* playback_timer_;
};

// TODO(mlamouri): test that visits are not recorded multiple times when a
// same-origin navigation happens.

TEST_F(MediaEngagementContentsObserverTest, SignificantActivePlayerCount) {
  EXPECT_EQ(0u, GetSignificantActivePlayersCount());

  SimulatePlaybackStarted(0);
  EXPECT_EQ(1u, GetSignificantActivePlayersCount());

  SimulatePlaybackStarted(1);
  EXPECT_EQ(2u, GetSignificantActivePlayersCount());

  SimulatePlaybackStarted(2);
  EXPECT_EQ(3u, GetSignificantActivePlayersCount());

  SimulatePlaybackStopped(1);
  EXPECT_EQ(2u, GetSignificantActivePlayersCount());

  SimulatePlaybackStopped(0);
  EXPECT_EQ(1u, GetSignificantActivePlayersCount());

  SimulatePlaybackStopped(2);
  EXPECT_EQ(0u, GetSignificantActivePlayersCount());
}

TEST_F(MediaEngagementContentsObserverTest, UpdateAutoplayOrigin) {
  GURL url1("https://www.google.com");
  GURL url2("https://www.example.org");

  // url1 should have a 1.0 score and url2 should have a 0.0 score.
  SetMetricsForOrigin(url1, MediaEngagementScore::kScoreMinVisits + 1,
                      MediaEngagementScore::kScoreMinVisits + 1);
  SetMetricsForOrigin(url2, 1, 1);
  EXPECT_EQ(1.0, GetEngagementScore(url1));
  EXPECT_EQ(0.0, GetEngagementScore(url2));

  NavigateAndCommit(url1);
  ExpectAutoplayOriginScope(main_rfh(), url1.spec());

  NavigateAndCommit(url2);
  ExpectAutoplayOriginScope(main_rfh(), "");
}

TEST_F(MediaEngagementContentsObserverTest, UpdateAutoplayOriginFrame) {
  GURL root("https://www.google.com");
  GURL child("https://www.testplayer.com");
  GURL other_root("https://www.example.org");

  SetMetricsForOrigin(root, MediaEngagementScore::kScoreMinVisits + 1,
                      MediaEngagementScore::kScoreMinVisits + 1);
  SetMetricsForOrigin(other_root, 1, 1);
  EXPECT_EQ(1.0, GetEngagementScore(root));
  EXPECT_EQ(0.0, GetEngagementScore(other_root));

  NavigateAndCommit(child);
  ExpectAutoplayOriginScope(main_rfh(), "");

  NavigateAndCommit(root);
  content::RenderFrameHost* child_rfh = CreateFrame(main_rfh(), child);
  ExpectAutoplayOriginScope(child_rfh, child.spec());

  NavigateAndCommit(other_root);
  child_rfh = CreateFrame(main_rfh(), child);
  ExpectAutoplayOriginScope(child_rfh, "");
}

TEST_F(MediaEngagementContentsObserverTest, AreConditionsMet) {
  EXPECT_FALSE(AreConditionsMet());

  SimulatePlaybackStarted(0);
  SimulateIsVisible();
  web_contents()->SetAudioMuted(false);
  EXPECT_TRUE(AreConditionsMet());

  web_contents()->SetAudioMuted(true);
  EXPECT_FALSE(AreConditionsMet());

  web_contents()->SetAudioMuted(false);
  SimulateIsHidden();
  EXPECT_FALSE(AreConditionsMet());

  SimulateIsVisible();
  SimulatePlaybackStopped(0);
  EXPECT_FALSE(AreConditionsMet());

  SimulatePlaybackStarted(0);
  EXPECT_TRUE(AreConditionsMet());

  SimulateMutedStateChange(0, true);
  EXPECT_FALSE(AreConditionsMet());

  SimulatePlaybackStarted(1);
  EXPECT_TRUE(AreConditionsMet());
}

TEST_F(MediaEngagementContentsObserverTest, EnsureCleanupAfterNavigation) {
  EXPECT_FALSE(GetStoredPlayerStatesCount());

  SimulateMutedStateChange(0, true);
  EXPECT_TRUE(GetStoredPlayerStatesCount());

  NavigateAndCommit(GURL("https://example.com"));
  EXPECT_FALSE(GetStoredPlayerStatesCount());
}

TEST_F(MediaEngagementContentsObserverTest, TimerRunsDependingOnConditions) {
  EXPECT_FALSE(IsTimerRunning());

  SimulatePlaybackStarted(0);
  SimulateIsVisible();
  web_contents()->SetAudioMuted(false);
  EXPECT_TRUE(IsTimerRunning());

  web_contents()->SetAudioMuted(true);
  EXPECT_FALSE(IsTimerRunning());

  web_contents()->SetAudioMuted(false);
  SimulateIsHidden();
  EXPECT_FALSE(IsTimerRunning());

  SimulateIsVisible();
  SimulatePlaybackStopped(0);
  EXPECT_FALSE(IsTimerRunning());

  SimulatePlaybackStarted(0);
  EXPECT_TRUE(IsTimerRunning());

  SimulateMutedStateChange(0, true);
  EXPECT_FALSE(IsTimerRunning());

  SimulatePlaybackStarted(1);
  EXPECT_TRUE(IsTimerRunning());
}

TEST_F(MediaEngagementContentsObserverTest, TimerDoesNotRunIfEntryRecorded) {
  SimulateSignificantPlaybackRecorded();

  SimulatePlaybackStarted(0);
  SimulateIsVisible();
  web_contents()->SetAudioMuted(false);

  EXPECT_FALSE(IsTimerRunning());
}

TEST_F(MediaEngagementContentsObserverTest,
       SignificantPlaybackRecordedWhenTimerFires) {
  SimulatePlaybackStarted(0);
  SimulateIsVisible();
  web_contents()->SetAudioMuted(false);
  EXPECT_TRUE(IsTimerRunning());
  EXPECT_FALSE(WasSignificantPlaybackRecorded());

  SimulatePlaybackTimerFired();
  EXPECT_TRUE(WasSignificantPlaybackRecorded());
}

TEST_F(MediaEngagementContentsObserverTest, InteractionsRecorded) {
  GURL url("https://www.google.com");
  ExpectScores(url, 0, 0);

  NavigateAndCommit(url);
  ExpectScores(url, 1, 0);

  SimulateSignificantPlaybackTime();
  ExpectScores(url, 1, 1);
}

TEST_F(MediaEngagementContentsObserverTest, DoNotRecordAudiolessTrack) {
  EXPECT_EQ(0u, GetSignificantActivePlayersCount());

  content::WebContentsObserver::MediaPlayerInfo player_info(true, false);
  SimulatePlaybackStarted(player_info, 0);
  EXPECT_EQ(0u, GetSignificantActivePlayersCount());
}
