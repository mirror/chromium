// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_contents_observer.h"

#include "chrome/browser/media/media_engagement_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

constexpr base::TimeDelta
    MediaEngagementContentsObserver::kSignificantMediaPlaybackTime;

MediaEngagementContentsObserver::MediaEngagementContentsObserver(
    content::WebContents* web_contents,
    MediaEngagementService* service)
    : WebContentsObserver(web_contents),
      service_(service),
      playback_timer_(new base::Timer(true, false)) {}

MediaEngagementContentsObserver::~MediaEngagementContentsObserver() = default;

void MediaEngagementContentsObserver::WebContentsDestroyed() {
  playback_timer_->Stop();
  service_->contents_observers_.erase(this);
  delete this;
}

void MediaEngagementContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument() || navigation_handle->IsErrorPage()) {
    return;
  }

  DCHECK(!playback_timer_->IsRunning());
  DCHECK(significant_players_.empty());

  url::Origin new_origin(navigation_handle->GetURL());
  if (committed_origin_.IsSameOriginWith(new_origin))
    return;

  committed_origin_ = new_origin;
  significant_playback_recorded_ = false;

  if (committed_origin_.unique())
    return;

  // TODO(mlamouri): record the visit into content settings.
}

void MediaEngagementContentsObserver::WasShown() {
  is_visible_ = true;
  UpdateTimer();
}

void MediaEngagementContentsObserver::WasHidden() {
  is_visible_ = false;
  UpdateTimer();
}

void MediaEngagementContentsObserver::MediaStartedPlaying(
    const MediaPlayerInfo& media_player_info,
    const MediaPlayerId& media_player_id) {
  // TODO(mlamouri): check if:
  // - the playback has the minimum size requirements;
  if (!media_player_info.has_audio)
    return;

  DCHECK(playing_players_.find(media_player_id) == playing_players_.end());
  playing_players_.insert(media_player_id);

  MaybeInsertSignificantPlayer(media_player_id);
  UpdateTimer();
}

void MediaEngagementContentsObserver::MediaVolumeChanged(
    const MediaPlayerId& id,
    double volume) {
  if (volume == 0.0) {
    MaybeRemoveSignificantPlayer(id);
  } else {
    MaybeInsertSignificantPlayer(id);
  }

  UpdateTimer();
}

void MediaEngagementContentsObserver::MediaStoppedPlaying(
    const MediaPlayerInfo& media_player_info,
    const MediaPlayerId& media_player_id) {
  DCHECK(playing_players_.find(media_player_id) != playing_players_.end());
  playing_players_.erase(media_player_id);

  MaybeRemoveSignificantPlayer(media_player_id);
  UpdateTimer();
}

void MediaEngagementContentsObserver::DidUpdateAudioMutingState(bool muted) {
  UpdateTimer();
}

void MediaEngagementContentsObserver::OnSignificantMediaPlaybackTime() {
  DCHECK(!significant_playback_recorded_);

  significant_playback_recorded_ = true;

  if (committed_origin_.unique())
    return;

  // TODO(mlamouri): record the playback into content settings.
}

void MediaEngagementContentsObserver::MaybeInsertSignificantPlayer(
    const MediaPlayerId& id) {
  auto player = significant_players_.find(id);
  if (player != significant_players_.end())
    return;

  // Check that the player is currently playing.
  auto is_playing = playing_players_.find(id);
  if (is_playing == playing_players_.end())
    return;

  // Check that the player is not muted.
  auto volume = web_contents()->GetMediaVolumes().find(id);
  if (volume == web_contents()->GetMediaVolumes().end() ||
      volume->second == 0.0)
    return;

  significant_players_.insert(id);
}

void MediaEngagementContentsObserver::MaybeRemoveSignificantPlayer(
    const MediaPlayerId& id) {
  auto player = significant_players_.find(id);
  if (player == significant_players_.end())
    return;
  bool significant = true;

  // If the player is no longer playing, it should not be significant.
  auto is_playing = playing_players_.find(id);
  if (is_playing == playing_players_.end())
    significant = false;

  // If the player is muted, or we don't know the volume then the player can't
  // be significant.
  auto volume = web_contents()->GetMediaVolumes().find(id);
  if (volume == web_contents()->GetMediaVolumes().end() ||
      volume->second == 0.0)
    significant = false;

  if (!significant)
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
