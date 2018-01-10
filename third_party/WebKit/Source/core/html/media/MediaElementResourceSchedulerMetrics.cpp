// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/media/MediaElementResourceSchedulerMetrics.h"

#include <utility>

#include "platform/Histogram.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/Time.h"

namespace {
// Limit the number of players so that we don't exceep the maximum number of UMA
// buckets in the histogram.  For simplicity, we always allow a zero bucket in
// the histograms.
constexpr size_t kMaxPlayers = 99;
}  // namespace

namespace blink {

MediaElementResourceSchedulerMetrics::PlayerRecord::PlayerRecord()
    : constructed_at(CurrentTimeTicksInMilliseconds()) {}
MediaElementResourceSchedulerMetrics::PlayerRecord::PlayerRecord(
    const PlayerRecord&) = default;

// static
MediaElementResourceSchedulerMetrics*
MediaElementResourceSchedulerMetrics::GetInstance() {
  static MediaElementResourceSchedulerMetrics* instance = nullptr;
  if (!instance)
    instance = new MediaElementResourceSchedulerMetrics();

  return instance;
}

MediaElementResourceSchedulerMetrics::MediaElementResourceSchedulerMetrics() =
    default;
MediaElementResourceSchedulerMetrics::~MediaElementResourceSchedulerMetrics() =
    default;

void MediaElementResourceSchedulerMetrics::OnPlayerCreated(
    HTMLMediaElement* element) {
  DCHECK(players_.find(element) == players_.end());

  players_.emplace(std::make_pair(element, PlayerRecord()));
}

void MediaElementResourceSchedulerMetrics::OnPlayerLoadStarted(
    HTMLMediaElement* element,
    int priority) {
  auto player_iter = players_.find(element);
  DCHECK(player_iter != players_.end());
  player_iter->second.priority = priority;

  // Record that a player was loaded, including whether it had a priority.
  DEFINE_STATIC_LOCAL(EnumerationHistogram, on_player_load_histogram,
                      ("Media.ResourceScheduler.OnPlayerLoad", 2));
  on_player_load_histogram.Count(player_iter->second.priority !=
                                 std::numeric_limits<int>::min());
}

void MediaElementResourceSchedulerMetrics::OnPlayerDestroyed(
    HTMLMediaElement* element) {
  // It's okay if |element| isn't registered.
  players_.erase(element);
}

void MediaElementResourceSchedulerMetrics::OnPlayerPlayed(
    HTMLMediaElement* element) {
  auto player_iter = players_.find(element);
  DCHECK(player_iter != players_.end());

  PlayerRecord& player_rec = player_iter->second;

  // We care only about the first playback.
  if (player_rec.has_been_played)
    return;

  player_rec.has_been_played = true;
  // Limit the total because UMA needs it.
  size_t total = std::min(players_.size(), kMaxPlayers);

  // Add one to the "total bucket", to record how many times we had |total|
  // players in the comparison.  Note that the |total==1| bucket will be
  // trivially zero right, zero wrong.
  DEFINE_STATIC_LOCAL(LinearHistogram, on_player_played_total_histogram,
                      ("Media.ResourceScheduler.OnPlayerPlayed.Total", 0,
                       kMaxPlayers, kMaxPlayers + 1));
  DEFINE_STATIC_LOCAL(LinearHistogram, on_player_played_right_histogram,
                      ("Media.ResourceScheduler.OnPlayerPlayed.Right", 0,
                       kMaxPlayers, kMaxPlayers + 1));
  DEFINE_STATIC_LOCAL(LinearHistogram, on_player_played_wrong_histogram,
                      ("Media.ResourceScheduler.OnPlayerPlayed.Wrong", 0,
                       kMaxPlayers, kMaxPlayers + 1));

  on_player_played_total_histogram.Count(total);

  for (auto& kv : players_) {
    PlayerRecord& other_rec = kv.second;

    bool is_higher_priority = other_rec.priority > player_rec.priority;

    // Order of equally prioritized player is undefined.
    if (other_rec.priority == player_rec.priority)
      continue;

    // If this player has already started playback, then assume that it won't
    // affect the RS decision.  We will treat the later playback of |this|
    // with higher priority.
    if (other_rec.has_been_played)
      continue;

    // This player hasn't played.  It should be lower priority than us, so
    // that it would receive resources to preroll sooner if possible.  Note that
    // we record one count in the |total| bucket, so that that the effect of
    // list size can be observed.  One must divide these by the number of counts
    // in the |total| bucket in the Media...Total histogram, above, in order to
    // make much sense of it, as a result.
    if (is_higher_priority)
      on_player_played_wrong_histogram.Count(total);
    else
      on_player_played_right_histogram.Count(total);
  }
}

void MediaElementResourceSchedulerMetrics::OnFirstFrameDrawn(
    HTMLMediaElement* element) {
  auto player_iter = players_.find(element);
  // This can sometimes happen after |player| has been destroyed, since it's
  // posted from another thread.  Just ignore it.
  if (player_iter == players_.end())
    return;

  PlayerRecord& player_rec = player_iter->second;

  DCHECK(!player_rec.first_frame_drawn_at);
  player_rec.first_frame_drawn_at = CurrentTimeTicksInMilliseconds();

  // Limit the total because UMA needs it.
  int total = std::min(players_.size(), kMaxPlayers);

  DEFINE_STATIC_LOCAL(LinearHistogram, on_first_frame_drawn_total_histogram,
                      ("Media.ResourceScheduler.OnFirstFrameDrawn.Total", 0,
                       kMaxPlayers, kMaxPlayers + 1));
  DEFINE_STATIC_LOCAL(LinearHistogram, on_first_frame_drawn_right_histogram,
                      ("Media.ResourceScheduler.OnFirstFrameDrawn.Right", 0,
                       kMaxPlayers, kMaxPlayers + 1));
  DEFINE_STATIC_LOCAL(LinearHistogram, on_first_frame_drawn_wrong_histogram,
                      ("Media.ResourceScheduler.OnFirstFrameDrawn.Wrong", 0,
                       kMaxPlayers, kMaxPlayers + 1));

  on_first_frame_drawn_total_histogram.Count(total);

  for (auto& kv : players_) {
    // Ignore things of equal priority, which includes us.  Either order is ok.
    PlayerRecord& other_rec = kv.second;
    if (other_rec.priority == player_rec.priority)
      continue;

    bool did_draw_sooner = !!other_rec.first_frame_drawn_at;
    bool completed_before_we_started =
        did_draw_sooner &&
        (other_rec.first_frame_drawn_at < player_rec.first_frame_drawn_at);

    // Ignore things that complete drawing a frame before we were created. The
    // resource scheduler probably shouldn't try to delay things that much.
    // Even if it did, it has the option to suspend the other player to preroll
    // us, but that's an entirely different action than we care about.  We care
    // about ordering things that are actually competing to get a frame on the
    // display at the same time.
    if (completed_before_we_started)
      continue;

    // If |iter| has started playing, then we probably shouldn't count it here.
    // We would increase its priority at that point.  Then again, maybe this is
    // too complicated.
    if (other_rec.has_been_played)
      continue;

    // If we were constructed first, then pretend that we're higher priority.
    // We would (maybe) be loaded first by the RS anyway, since it wouldn't know
    // about |iter|.  However, it's a bit unfair to the RS, since it's possible
    // that we would be deferred due to previous RS decisions until after |iter|
    // is constructed.  We go for the more conservative approach here.
    // "Conservative" isn't quite right, either, since it lets us get things
    // right by accident -- if they are constructed earlier and start sooner,
    // then order doesn't really matter and we still get it right.
    bool is_higher_priority = other_rec.priority > player_rec.priority;

    // We ordered these two right if and only if they drew in priority order.
    // We include a count in the |total| bucket, so that the effect of total
    // list size can be observed.  Divide by Media...Total to make sense of it.
    if (is_higher_priority != did_draw_sooner) {
      // Either they're higher priority but haven't drawn yet, or we are and
      // they did.
      on_first_frame_drawn_wrong_histogram.Count(total);
    } else {
      on_first_frame_drawn_right_histogram.Count(total);
    }
  }
}

}  // namespace blink
