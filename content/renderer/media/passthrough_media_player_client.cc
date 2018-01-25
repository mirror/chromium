// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/passthrough_media_player_client.h"

using blink::WebInbandTextTrack;
using blink::WebLayer;
using blink::WebLocalizedString;
using blink::WebMediaPlayer;
using blink::WebMediaPlayerClient;
using blink::WebMediaPlayerSource;
using blink::WebMediaSource;
using blink::WebRemotePlaybackAvailability;
using blink::WebRemotePlaybackClient;
using blink::WebString;
using blink::WebURL;

namespace content {

PassthroughMediaPlayerClient::PassthroughMediaPlayerClient(
    WebMediaPlayerClient* client)
    : client_(client) {}

void PassthroughMediaPlayerClient::NetworkStateChanged() {
  client_->NetworkStateChanged();
}

void PassthroughMediaPlayerClient::ReadyStateChanged() {
  client_->ReadyStateChanged();
}

void PassthroughMediaPlayerClient::TimeChanged() {
  client_->TimeChanged();
}

void PassthroughMediaPlayerClient::Repaint() {
  client_->Repaint();
}

void PassthroughMediaPlayerClient::DurationChanged() {
  client_->DurationChanged();
}

void PassthroughMediaPlayerClient::SizeChanged() {
  client_->SizeChanged();
}

void PassthroughMediaPlayerClient::PlaybackStateChanged() {
  client_->PlaybackStateChanged();
}

void PassthroughMediaPlayerClient::SetWebLayer(WebLayer* web_layer) {
  client_->SetWebLayer(web_layer);
}

WebMediaPlayer::TrackId PassthroughMediaPlayerClient::AddAudioTrack(
    const WebString& id,
    AudioTrackKind kind,
    const WebString& label,
    const WebString& language,
    bool enabled) {
  return client_->AddAudioTrack(id, kind, label, language, enabled);
}

void PassthroughMediaPlayerClient::RemoveAudioTrack(
    WebMediaPlayer::TrackId track_id) {
  client_->RemoveAudioTrack(track_id);
}

WebMediaPlayer::TrackId PassthroughMediaPlayerClient::AddVideoTrack(
    const WebString& id,
    VideoTrackKind kind,
    const WebString& label,
    const WebString& language,
    bool selected) {
  return client_->AddVideoTrack(id, kind, label, language, selected);
}

void PassthroughMediaPlayerClient::RemoveVideoTrack(
    WebMediaPlayer::TrackId track_id) {
  client_->RemoveVideoTrack(track_id);
}

void PassthroughMediaPlayerClient::AddTextTrack(WebInbandTextTrack* track) {
  client_->AddTextTrack(track);
}

void PassthroughMediaPlayerClient::RemoveTextTrack(WebInbandTextTrack* track) {
  client_->RemoveTextTrack(track);
}

void PassthroughMediaPlayerClient::MediaSourceOpened(WebMediaSource* source) {
  client_->MediaSourceOpened(source);
}

void PassthroughMediaPlayerClient::RequestSeek(double time) {
  client_->RequestSeek(time);
}

void PassthroughMediaPlayerClient::RemoteRouteAvailabilityChanged(
    WebRemotePlaybackAvailability availability) {
  return client_->RemoteRouteAvailabilityChanged(availability);
}

void PassthroughMediaPlayerClient::ConnectedToRemoteDevice() {
  client_->ConnectedToRemoteDevice();
}

void PassthroughMediaPlayerClient::DisconnectedFromRemoteDevice() {
  client_->DisconnectedFromRemoteDevice();
}

void PassthroughMediaPlayerClient::CancelledRemotePlaybackRequest() {
  client_->CancelledRemotePlaybackRequest();
}

void PassthroughMediaPlayerClient::RemotePlaybackStarted() {
  client_->RemotePlaybackStarted();
}

void PassthroughMediaPlayerClient::RemotePlaybackCompatibilityChanged(
    const WebURL& url,
    bool is_compatible) {
  client_->RemotePlaybackCompatibilityChanged(url, is_compatible);
}

void PassthroughMediaPlayerClient::OnBecamePersistentVideo(bool is_persistent) {
  client_->OnBecamePersistentVideo(is_persistent);
}

void PassthroughMediaPlayerClient::ActivateViewportIntersectionMonitoring(
    bool activate) {
  client_->ActivateViewportIntersectionMonitoring(activate);
}

bool PassthroughMediaPlayerClient::IsAutoplayingMuted() {
  return client_->IsAutoplayingMuted();
}

bool PassthroughMediaPlayerClient::HasSelectedVideoTrack() {
  return client_->HasSelectedVideoTrack();
}

WebMediaPlayer::TrackId
PassthroughMediaPlayerClient::GetSelectedVideoTrackId() {
  return client_->GetSelectedVideoTrackId();
}

void PassthroughMediaPlayerClient::MediaRemotingStarted(
    const WebString& remote_device_friendly_name) {
  client_->MediaRemotingStarted(remote_device_friendly_name);
}

void PassthroughMediaPlayerClient::MediaRemotingStopped(
    WebLocalizedString::Name error_msg) {
  client_->MediaRemotingStopped(std::move(error_msg));
}

void PassthroughMediaPlayerClient::PictureInPictureStarted() {
  client_->PictureInPictureStarted();
}

void PassthroughMediaPlayerClient::PictureInPictureStopped() {
  client_->PictureInPictureStopped();
}

bool PassthroughMediaPlayerClient::HasNativeControls() {
  return client_->HasNativeControls();
}

bool PassthroughMediaPlayerClient::IsAudioElement() {
  return client_->IsAudioElement();
}

WebMediaPlayer::DisplayType PassthroughMediaPlayerClient::DisplayType() const {
  return client_->DisplayType();
}

WebRemotePlaybackClient* PassthroughMediaPlayerClient::RemotePlaybackClient() {
  return client_->RemotePlaybackClient();
}

gfx::ColorSpace PassthroughMediaPlayerClient::TargetColorSpace() {
  return client_->TargetColorSpace();
}

}  // namespace content
