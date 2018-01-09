// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaElementResourceSchedulerMetrics_h
#define MediaElementResourceSchedulerMetrics_h

#include <map>

#include "platform/wtf/Optional.h"

namespace blink {

class HTMLMediaElement;

// Class to gather metrics about the effectiveness of a priority assignment to
// WebMediaPlayer instances.
class MediaElementResourceSchedulerMetrics final {
 public:
  static MediaElementResourceSchedulerMetrics* GetInstance();

  // Called when a WebMediaPlayer is created for the element.  We track the
  // element instead of the player itself, just because it's much easier to
  // ensure that the element notifies us on destruction.  It is okay for the
  // same element to call OnPlayerCreated twice if it calls OnPlayerDestroyed
  // between them.
  void OnPlayerCreated(HTMLMediaElement*);

  // |priority| should be numerically higher for more important players.  It is
  // okay for players to share a priority; we won't prefer one to the other.
  void OnPlayerLoadStarted(HTMLMediaElement*, int priority);

  // Notifies us that the player for the element has been destroyed.
  void OnPlayerDestroyed(HTMLMediaElement*);

  void OnPlayerPlayed(HTMLMediaElement*);

  // This call may occur after the element has been destroyed, in which case
  // we'll ignore it.
  void OnFirstFrameDrawn(HTMLMediaElement*);

 private:
  MediaElementResourceSchedulerMetrics();
  ~MediaElementResourceSchedulerMetrics();

  // The state of the player.
  struct PlayerRecord {
    PlayerRecord();
    PlayerRecord(const PlayerRecord& other);

    // Time of OnPlayerCreated.
    double constructed_at = 0;

    // Priority at construction.
    int priority = std::numeric_limits<int>::min();

    // If filled in, the time of OnFirstFrameDrawn.
    WTF::Optional<double> first_frame_drawn_at;

    // Has this player started playing at least once?
    bool has_been_played = false;
  };

  std::map<HTMLMediaElement*, PlayerRecord> players_;

  DISALLOW_COPY_AND_ASSIGN(MediaElementResourceSchedulerMetrics);
};

}  // namespace blink

#endif  // MediaElementResourceSchedulerMetrics_h
