// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_contents_observer.h"

#include "chrome/browser/media/media_engagement_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "media/base/media_switches.h"

constexpr base::TimeDelta
    MediaEngagementContentsObserver::kSignificantMediaPlaybackTime;

namespace {

GURL GetURLOfMainFrame(content::RenderFrameHost* render_frame_host) {
  if (render_frame_host->GetParent())
    return GetURLOfMainFrame(render_frame_host->GetParent());
  return render_frame_host->GetLastCommittedURL();
}

}  // namespace

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
}

void MediaEngagementContentsObserver::UpdateAutoplayOrigin(
    content::RenderFrameHost* render_frame_host) {
  if (!base::FeatureList::IsEnabled(
          media::kMediaEngagementBypassAutoplayPolicies))
    return;

  // If the frame is an iFrame, we should find the URL of the main frame and use
  // that to check the engagement score. If the score is high enough to bypass
  // autopolicy policies then the new whitelist scope should be the origin of
  // the current page.
  std::string new_value = "";
  if (service_->OriginIsAllowedToBypassAutoplayPolicy(
          GetURLOfMainFrame(render_frame_host))) {
    new_value = render_frame_host->GetLastCommittedOrigin().Serialize();
  }

  content::RenderViewHost* render_view_host =
      render_frame_host->GetRenderViewHost();
  content::WebPreferences preferences =
      render_view_host->GetWebkitPreferences();
  if (new_value != preferences.media_playback_gesture_whitelist_scope) {
    preferences.media_playback_gesture_whitelist_scope = new_value;
    render_view_host->UpdateWebkitPreferences(preferences);
  }
}

void MediaEngagementContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument() || navigation_handle->IsErrorPage()) {
    return;
  }

  UpdateAutoplayOrigin(navigation_handle->GetRenderFrameHost());

  if (!navigation_handle->IsInMainFrame())
    return;

  DCHECK(!playback_timer_->IsRunning());
  DCHECK(significant_players_.empty());

  ClearPlayerStates();

  url::Origin new_origin(navigation_handle->GetURL());
  if (committed_origin_.IsSameOriginWith(new_origin))
    return;

  committed_origin_ = new_origin;
  significant_playback_recorded_ = false;

  if (committed_origin_.unique())
    return;
  service_->HandleInteraction(committed_origin_.GetURL(),
                              MediaEngagementService::INTERACTION_VISIT);
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
  // TODO(mlamouri): check if:
  // - the playback has the minimum size requirements;
  if (!media_player_info.has_audio)
    return;

  GetPlayerState(media_player_id)->playing = true;
  MaybeInsertSignificantPlayer(media_player_id);
  UpdateTimer();
}

void MediaEngagementContentsObserver::MediaMutedStateChanged(
    const MediaPlayerId& id,
    bool muted_state) {
  GetPlayerState(id)->muted = muted_state;

  if (muted_state)
    MaybeRemoveSignificantPlayer(id);
  else
    MaybeInsertSignificantPlayer(id);

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

bool MediaEngagementContentsObserver::IsSignificantPlayer(
    const MediaPlayerId& id) {
  PlayerState* state = GetPlayerState(id);
  return !state->muted && state->playing;
}

void MediaEngagementContentsObserver::OnSignificantMediaPlaybackTime() {
  DCHECK(!significant_playback_recorded_);

  significant_playback_recorded_ = true;

  if (committed_origin_.unique())
    return;

  service_->HandleInteraction(committed_origin_.GetURL(),
                              MediaEngagementService::INTERACTION_MEDIA_PLAYED);
}

void MediaEngagementContentsObserver::MaybeInsertSignificantPlayer(
    const MediaPlayerId& id) {
  if (significant_players_.find(id) != significant_players_.end())
    return;

  if (IsSignificantPlayer(id))
    significant_players_.insert(id);
}

void MediaEngagementContentsObserver::MaybeRemoveSignificantPlayer(
    const MediaPlayerId& id) {
  if (significant_players_.find(id) == significant_players_.end())
    return;

  if (!IsSignificantPlayer(id))
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
