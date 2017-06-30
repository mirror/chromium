// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SERVICE_H_
#define CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SERVICE_H_

#include <set>

#include "base/macros.h"
#include "chrome/browser/media/media_engagement_score.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class MediaEngagementContentsObserver;
class Profile;

namespace base {
class Clock;
}

namespace content {
class WebContents;
}  // namespace content

class MediaEngagementService : public KeyedService {
 public:
  // Returns the instance attached to the given |profile|.
  static MediaEngagementService* Get(Profile* profile);

  // Returns whether the feature is enabled.
  static bool IsEnabled();

  // Observe the given |web_contents| by creating an internal
  // WebContentsObserver.
  static void CreateWebContentsObserver(content::WebContents* web_contents);

  explicit MediaEngagementService(Profile* profile);
  ~MediaEngagementService() override;

  // Returns the engagement score of |url|.
  double GetEngagementScore(const GURL& url) const;

  // Returns a map of all stored origins and their engagement levels.
  std::map<GURL, double> GetScoreMap() const;

  enum InteractionTypes {
    INTERACTION_VISIT = 1 << 0,         // The URL was visited.
    INTERACTION_MEDIA_PLAYED = 1 << 1,  // Media was played on the URL.
  };

  // Update the engagement score of |url| for a combination of interactions.
  // |interactions| is a bitwise OR of InteractionTypes.
  void HandleInteraction(const GURL& url, unsigned short int interactions);

  // Returns true if the origin is allowed to bypass autoplay policies based on
  // the engagement score.
  bool OriginIsAllowedToBypassAutoplayPolicy(const GURL& url) const;

  // Origins with scores higher than this will be allowed to bypass autoplay
  // policies.
  const double kScoreAutoplayAllowed = 0.7;

 private:
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementServiceTest,
                           RestrictedToHTTPAndHTTPS);
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementServiceTest, HandleInteraction);
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementServiceTest,
                           IncognitoEngagementService);
  FRIEND_TEST_ALL_PREFIXES(MediaEngagementServiceTest,
                           CleanupOriginsOnHistoryDeletion);
  friend class MediaEngagementServiceTest;
  friend class MediaEngagementContentsObserverTest;
  friend MediaEngagementContentsObserver;

  MediaEngagementService(Profile* profile, std::unique_ptr<base::Clock> clock);

  std::set<MediaEngagementContentsObserver*> contents_observers_;

  Profile* profile_;

  // An internal clock for testing.
  std::unique_ptr<base::Clock> clock_;

  // Returns true if we should record engagement for this url. Currently,
  // engagement is only earned for HTTP and HTTPS.
  bool ShouldRecordEngagement(const GURL& url) const;

  // Retrieves the MediaEngagementScore for |url|.
  MediaEngagementScore CreateEngagementScore(const GURL& url) const;

  DISALLOW_COPY_AND_ASSIGN(MediaEngagementService);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_SERVICE_H_
