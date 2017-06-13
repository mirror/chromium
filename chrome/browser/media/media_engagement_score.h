// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SCORE_H_
#define CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SCORE_H_

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "url/gurl.h"

class HostContentSettingsMap;

class MediaEngagementScore {
 public:
  MediaEngagementScore(const GURL& origin, HostContentSettingsMap* settings);

  MediaEngagementScore(const GURL& origin,
                       std::unique_ptr<base::DictionaryValue> score_dict);

  MediaEngagementScore(MediaEngagementScore&& other);
  ~MediaEngagementScore();
  MediaEngagementScore& operator=(MediaEngagementScore&& other);

  // Returns the total score, as per the formula in the design doc.
  double GetTotalScore() const;

  // Writes the values in this score into |settings_map_|. If there are multiple
  // instances of a score object for an origin, this could result in stale data
  // being stored.
  void Commit();

  // Get/increment the number of visits this origin has had.
  int visits() const { return visits_; }
  void increment_visits() { visits_++; }

  // Get/increment the number of media playbacks this origin has had.
  int media_playbacks() const { return media_playbacks_; }
  void increment_media_playbacks() { media_playbacks_++; }

  // Calculate the score as per the formula in the design doc.
  static double CalculateScore(int visits, int media_playbacks);

 private:
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementScoreTest, PartiallyEmptyDictionary);
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementScoreTest, PopulatedDictionary);
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementScoreTest,
                           ContentSettingsMultiOrigin);
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementScoreTest, TotalScoreCalculation);
  friend class MediaEngagementScoreTest;

  // The dictionary keys to store individual metrics. kVisitsKey will
  // store the number of visits to an origin and kMediaPlaybacksKey
  // will store the number of media playbacks on an origin.
  static const char* kVisitsKey;
  static const char* kMediaPlaybacksKey;

  // Origins with a number of visits less than this number will recieve
  // a score of zero.
  static const int kScoreMinVisits;

  // The number of media playbacks this origin has had.
  int media_playbacks_;

  // The number of visits this origin has had.
  int visits_;

  // The origin this score represents.
  GURL origin_;

  // The dictionary that represents this engagement score.
  std::unique_ptr<base::DictionaryValue> score_dict_;

  // The content settings map that will persist the score.
  HostContentSettingsMap* settings_map_;

  // Update the dictionary continaing the latest score values and return whether
  // they have changed or not (since what was last retrieved from content
  // settings).
  bool UpdateScoreDict(base::DictionaryValue* score_dict);

  // Get the dictionary containing the score values from content settings.
  static std::unique_ptr<base::DictionaryValue> GetScoreDictForSettings(
      const HostContentSettingsMap* settings,
      const GURL& origin_url);

  DISALLOW_COPY_AND_ASSIGN(MediaEngagementScore);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SCORE_H_
