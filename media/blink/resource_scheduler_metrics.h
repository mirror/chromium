// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_RESOURCE_SCHEDULER_METRICS_H_
#define MEDIA_BLINK_RESOURCE_SCHEDULER_METRICS_H_

#include <map>

#include "base/optional.h"
#include "base/timer/timer.h"
#include "media/blink/media_blink_export.h"

namespace blink {
class WebMediaPlayer;
}  // namespace blink

namespace media {

// Class to gather metrics about the effectiveness of a priority assignment to
// WebMediaPlayer instances.
class MEDIA_BLINK_EXPORT ResourceSchedulerMetrics {
 public:
  static ResourceSchedulerMetrics* GetInstance();

  void OnPlayerCreated(blink::WebMediaPlayer* player);
  // |priority| should be numerically higher for more important players.  It is
  // okay for players to share a priority; we won't prefer one to the other.
  void OnPlayerLoadStarted(blink::WebMediaPlayer* player, int priority);
  void OnPlayerDestroyed(blink::WebMediaPlayer* player);
  void OnPlayerPlayed(blink::WebMediaPlayer* player);

  // This call may occur after |player| has been destroyed, in which case
  // we'll ignore it.
  void OnFirstFrameDrawn(blink::WebMediaPlayer* player);

 private:
  ResourceSchedulerMetrics();
  ~ResourceSchedulerMetrics();

  // The state of the player.
  struct PlayerRecord {
    PlayerRecord();
    PlayerRecord(const PlayerRecord& other);

    // Time of OnPlayerCreated.
    base::TimeTicks constructed_at;

    // Priority at construction.
    float priority = INT_MIN;

    // If filled in, the time of OnFirstFrameDrawn.
    base::Optional<base::TimeTicks> first_frame_drawn_at;

    // Has this player started playing at least once?
    bool has_been_played = false;
  };

  std::map<blink::WebMediaPlayer*, PlayerRecord> players_;

  DISALLOW_COPY_AND_ASSIGN(ResourceSchedulerMetrics);
};

}  // namespace media

#endif  // MEDIA_BLINK_RESOURCE_SCHEDULER_METRICS_H_
