// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_contents_observer.h"

#include "base/metrics/histogram.h"
#include "build/build_config.h"
#include "chrome/browser/media/media_engagement_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {

constexpr base::TimeDelta kSignificantMediaPlaybackTime =
    base::TimeDelta::FromSeconds(7);

static int kMaxInsignificantPlaybackReason =
    static_cast<int>(InsignificantPlaybackReason::kReasonMax);

}  // namespace.

// This is the minimum size (in px) of each dimension that a media
// element has to be in order to be determined significant.
const gfx::Size MediaEngagementContentsObserver::kSignificantSize =
    gfx::Size(200, 140);

const char* MediaEngagementContentsObserver::kHistogramSignificantNotAddedName =
    "Media.Engagement.SignificantPlayers.PlayerNotAdded";

const char* MediaEngagementContentsObserver::kHistogramSignificantRemovedName =
    "Media.Engagement.SignificantPlayers.PlayerRemoved";

MediaEngagementContentsObserver::MediaEngagementContentsObserver(
    content::WebContents* web_contents,
    MediaEngagementService* service)
    : WebContentsObserver(web_contents),
      service_(service),
      playback_timer_(new base::Timer(true, false)) {}

MediaEngagementContentsObserver::~MediaEngagementContentsObserver() = default;

void MediaEngagementContentsObserver::WebContentsDestroyed() {
  playback_timer_->Stop();
  ClearPlayerStates();
  service_->contents_observers_.erase(this);
  delete this;
}

void MediaEngagementContentsObserver::ClearPlayerStates() {
  for (auto const& p : player_states_)
    delete p.second;
  player_states_.clear();
  significant_players_.clear();
}

void MediaEngagementContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument() || navigation_handle->IsErrorPage()) {
    return;
  }

  playback_timer_->Stop();
  ClearPlayerStates();

  url::Origin new_origin(navigation_handle->GetURL());
  if (committed_origin_.IsSameOriginWith(new_origin))
    return;

  committed_origin_ = new_origin;
  significant_playback_recorded_ = false;

  if (committed_origin_.unique())
    return;

  service_->RecordVisit(committed_origin_.GetURL());
}

void MediaEngagementContentsObserver::WasShown() {
  is_visible_ = true;
  UpdateTimer();
}

void MediaEngagementContentsObserver::WasHidden() {
  is_visible_ = false;
  UpdateTimer();
}

MediaEngagementContentsObserver::PlayerState*
MediaEngagementContentsObserver::GetPlayerState(const MediaPlayerId& id) {
  auto state = player_states_.find(id);
  if (state != player_states_.end())
    return state->second;

  PlayerState* new_state = new PlayerState();
  player_states_[id] = new_state;
  return new_state;
}

void MediaEngagementContentsObserver::MediaStartedPlaying(
    const MediaPlayerInfo& media_player_info,
    const MediaPlayerId& media_player_id) {
  PlayerState* state = GetPlayerState(media_player_id);
  state->playing = true;
  state->has_audio = media_player_info.has_audio;

  MaybeInsertSignificantPlayer(media_player_id);
  UpdateTimer();
}

void MediaEngagementContentsObserver::MediaMutedStatusChanged(
    const MediaPlayerId& id,
    bool muted) {
  GetPlayerState(id)->muted = muted;

  if (muted)
    MaybeRemoveSignificantPlayer(id);
  else
    MaybeInsertSignificantPlayer(id);

  UpdateTimer();
}

void MediaEngagementContentsObserver::MediaResized(const gfx::Size& size,
                                                   const MediaPlayerId& id) {
  if (size.width() >= kSignificantSize.width() &&
      size.height() >= kSignificantSize.height()) {
    GetPlayerState(id)->significant_size = true;
    MaybeInsertSignificantPlayer(id);
  } else {
    GetPlayerState(id)->significant_size = false;
    MaybeRemoveSignificantPlayer(id);
  }

  UpdateTimer();
}

void MediaEngagementContentsObserver::MediaStoppedPlaying(
    const MediaPlayerInfo& media_player_info,
    const MediaPlayerId& media_player_id) {
  GetPlayerState(media_player_id)->playing = false;
  MaybeRemoveSignificantPlayer(media_player_id);
  UpdateTimer();
}

void MediaEngagementContentsObserver::DidUpdateAudioMutingState(bool muted) {
  UpdateTimer();
}

std::vector<InsignificantPlaybackReason>
MediaEngagementContentsObserver::GetInsignificantPlayerReason(
    const MediaPlayerId& id) {
  PlayerState* state = GetPlayerState(id);
  std::vector<InsignificantPlaybackReason> reasons;

  if (state->muted)
    reasons.push_back(InsignificantPlaybackReason::kAudioMuted);

  if (!state->playing)
    reasons.push_back(InsignificantPlaybackReason::kMediaPaused);

  if (!state->significant_size)
    reasons.push_back(InsignificantPlaybackReason::kFrameSizeTooSmall);

  if (!state->has_audio)
    reasons.push_back(InsignificantPlaybackReason::kNoAudioTrack);

  return reasons;
}

void MediaEngagementContentsObserver::OnSignificantMediaPlaybackTime() {
  DCHECK(!significant_playback_recorded_);

  // Do not record significant playback if the tab did not make
  // a sound in the last two seconds.
#if defined(OS_ANDROID)
// Skipping WasRecentlyAudible check on Android (not available).
#else
  if (!web_contents()->WasRecentlyAudible())
    return;
#endif

  significant_playback_recorded_ = true;

  if (committed_origin_.unique())
    return;

  service_->RecordPlayback(committed_origin_.GetURL());
}

bool MediaEngagementContentsObserver::IsSignificantPlayerAndRecord(
    const MediaPlayerId& id,
    InsignificantHistogram histogram) {
  std::vector<InsignificantPlaybackReason> reasons =
      GetInsignificantPlayerReason(id);

  base::HistogramBase* base_histogram = base::Histogram::FactoryGet(
      histogram == InsignificantHistogram::kPlayerRemoved
          ? MediaEngagementContentsObserver::kHistogramSignificantRemovedName
          : MediaEngagementContentsObserver::kHistogramSignificantNotAddedName,
      0, kMaxInsignificantPlaybackReason, kMaxInsignificantPlaybackReason + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag);

  for (auto reason : reasons) {
    base_histogram->Add(static_cast<int>(reason));
  }

  if (reasons.empty())
    return true;

  base_histogram->Add(static_cast<int>(InsignificantPlaybackReason::kCount));
  return false;
}

void MediaEngagementContentsObserver::MaybeInsertSignificantPlayer(
    const MediaPlayerId& id) {
  if (significant_players_.find(id) != significant_players_.end())
    return;

  if (IsSignificantPlayerAndRecord(id, InsignificantHistogram::kPlayerNotAdded))
    significant_players_.insert(id);
}

void MediaEngagementContentsObserver::MaybeRemoveSignificantPlayer(
    const MediaPlayerId& id) {
  if (significant_players_.find(id) == significant_players_.end())
    return;

  if (!IsSignificantPlayerAndRecord(id, InsignificantHistogram::kPlayerRemoved))
    significant_players_.erase(id);
}

bool MediaEngagementContentsObserver::AreConditionsMet() const {
  if (significant_players_.empty() || !is_visible_)
    return false;

  return !web_contents()->IsAudioMuted();
}

void MediaEngagementContentsObserver::UpdateTimer() {
  if (significant_playback_recorded_)
    return;

  if (AreConditionsMet()) {
    if (playback_timer_->IsRunning())
      return;

    playback_timer_->Start(
        FROM_HERE, kSignificantMediaPlaybackTime,
        base::Bind(
            &MediaEngagementContentsObserver::OnSignificantMediaPlaybackTime,
            base::Unretained(this)));
  } else {
    if (!playback_timer_->IsRunning())
      return;
    playback_timer_->Stop();
  }
}

void MediaEngagementContentsObserver::SetTimerForTest(
    std::unique_ptr<base::Timer> timer) {
  playback_timer_ = std::move(timer);
}
