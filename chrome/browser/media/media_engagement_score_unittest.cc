// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_score.h"

#include <utility>

#include "base/macros.h"
#include "base/test/simple_test_clock.h"
#include "base/values.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

base::Time GetReferenceTime() {
  base::Time::Exploded exploded_reference_time;
  exploded_reference_time.year = 2015;
  exploded_reference_time.month = 1;
  exploded_reference_time.day_of_month = 30;
  exploded_reference_time.day_of_week = 5;
  exploded_reference_time.hour = 11;
  exploded_reference_time.minute = 0;
  exploded_reference_time.second = 0;
  exploded_reference_time.millisecond = 0;

  base::Time out_time;
  EXPECT_TRUE(
      base::Time::FromLocalExploded(exploded_reference_time, &out_time));
  return out_time;
}

}  // namespace

class MediaEngagementScoreTest : public ChromeRenderViewHostTestHarness {
 public:
  MediaEngagementScoreTest() : score_(&test_clock, GURL(), nullptr) {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    test_clock.SetNow(GetReferenceTime());
  }

  base::SimpleTestClock test_clock;

 protected:
  MediaEngagementScore score_;

  void VerifyScore(const MediaEngagementScore& score,
                   int expected_visits,
                   int expected_media_playbacks,
                   base::Time expected_last_media_playback_time) {
    EXPECT_EQ(expected_visits, score.visits());
    EXPECT_EQ(expected_media_playbacks, score.media_playbacks());
    EXPECT_EQ(expected_last_media_playback_time,
              score.last_media_playback_time());
  }

  void UpdateScore(MediaEngagementScore* score) {
    test_clock.SetNow(test_clock.Now() + base::TimeDelta::FromHours(1));

    score->increment_visits();
    score->increment_media_playbacks();
  }

  void TestScoreInitializesAndUpdates(
      std::unique_ptr<base::DictionaryValue> score_dict,
      int expected_visits,
      int expected_media_playbacks,
      base::Time expected_last_media_playback_time) {
    std::unique_ptr<base::DictionaryValue> copy(score_dict->DeepCopy());
    MediaEngagementScore initial_score(&test_clock, GURL(),
                                       std::move(score_dict));
    VerifyScore(initial_score, expected_visits, expected_media_playbacks,
                expected_last_media_playback_time);

    // Updating the score dict should return false, as the score shouldn't
    // have changed at this point.
    EXPECT_FALSE(initial_score.UpdateScoreDict(copy.get()));

    // Increment the scores and check that the values were stored correctly.
    UpdateScore(&initial_score);
    EXPECT_TRUE(initial_score.UpdateScoreDict(copy.get()));
    MediaEngagementScore updated_score(&test_clock, GURL(), std::move(copy));
    VerifyScore(updated_score, initial_score.visits(),
                initial_score.media_playbacks(), test_clock.Now());
  }

  void VerifyGetDetails(MediaEngagementScore* score) {
    mojom::MediaEngagementDetails details = score->GetDetails();
    EXPECT_EQ(details.origin, score->origin_);
    EXPECT_EQ(details.total_score, score->GetTotalScore());
    EXPECT_EQ(details.visits, score->visits());
    EXPECT_EQ(details.media_playbacks, score->media_playbacks());
    EXPECT_EQ(details.last_media_playback_time,
              score->last_media_playback_time().ToJsTime());
  }
};

// Test Mojo serialization.
TEST_F(MediaEngagementScoreTest, MojoSerialization) {
  VerifyGetDetails(&score_);
  UpdateScore(&score_);
  VerifyGetDetails(&score_);
}

// Test that scores are read / written correctly from / to empty score
// dictionaries.
TEST_F(MediaEngagementScoreTest, EmptyDictionary) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  TestScoreInitializesAndUpdates(std::move(dict), 0, 0, base::Time());
}

// Test that scores are read / written correctly from / to partially empty
// score dictionaries.
TEST_F(MediaEngagementScoreTest, PartiallyEmptyDictionary) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetInteger(MediaEngagementScore::kVisitsKey, 2);

  TestScoreInitializesAndUpdates(std::move(dict), 2, 0, base::Time());
}

// Test that scores are read / written correctly from / to populated score
// dictionaries.
TEST_F(MediaEngagementScoreTest, PopulatedDictionary) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetInteger(MediaEngagementScore::kVisitsKey, 1);
  dict->SetInteger(MediaEngagementScore::kMediaPlaybacksKey, 2);
  dict->SetDouble(MediaEngagementScore::kLastMediaPlaybackTimeKey,
                  test_clock.Now().ToInternalValue());

  TestScoreInitializesAndUpdates(std::move(dict), 1, 2, test_clock.Now());
}

// Test getting and commiting the score works correctly with different
// origins.
TEST_F(MediaEngagementScoreTest, ContentSettingsMultiOrigin) {
  GURL url("http://www.google.com");

  // Replace |score_| with one with an actual URL, and with a settings map.
  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile());
  score_ = MediaEngagementScore(&test_clock, url, settings_map);

  // Verify the score is originally zero, try incrementing and storing
  // the score.
  VerifyScore(score_, 0, 0, base::Time());
  score_.increment_visits();
  UpdateScore(&score_);
  score_.Commit();

  // Now confirm the correct score is present on the same origin,
  // but zero for a different origin.
  GURL same_origin("http://www.google.com/page1");
  GURL different_origin("http://www.google.co.uk");
  score_ = MediaEngagementScore(&test_clock, url, settings_map);
  MediaEngagementScore same_origin_score =
      MediaEngagementScore(&test_clock, same_origin, settings_map);
  MediaEngagementScore different_origin_score =
      MediaEngagementScore(&test_clock, different_origin, settings_map);
  VerifyScore(score_, 2, 1, test_clock.Now());
  VerifyScore(same_origin_score, 2, 1, test_clock.Now());
  VerifyScore(different_origin_score, 0, 0, base::Time());
}

// Test that the total score is calculated correctly.
TEST_F(MediaEngagementScoreTest, TotalScoreCalculation) {
  EXPECT_EQ(0, score_.GetTotalScore());
  UpdateScore(&score_);
  EXPECT_EQ(0, score_.GetTotalScore());
  UpdateScore(&score_);
  score_.visits_ = MediaEngagementScore::kScoreMinVisits;
  double expected_score =
      score_.media_playbacks() / (double)MediaEngagementScore::kScoreMinVisits;
  EXPECT_EQ(expected_score, score_.GetTotalScore());
}
