// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_CLIENT_H_
#define CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_CLIENT_H_

#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"

namespace content {

class PassthroughMediaPlayerClient : public blink::WebMediaPlayerClient {
 public:
  PassthroughMediaPlayerClient(blink::WebMediaPlayerClient* client);

  void NetworkStateChanged() override;
  void ReadyStateChanged() override;
  void TimeChanged() override;
  void Repaint() override;
  void DurationChanged() override;
  void SizeChanged() override;
  void PlaybackStateChanged() override;
  void SetWebLayer(blink::WebLayer*) override;
  blink::WebMediaPlayer::TrackId AddAudioTrack(const blink::WebString& id,
                                               AudioTrackKind,
                                               const blink::WebString& label,
                                               const blink::WebString& language,
                                               bool enabled) override;
  void RemoveAudioTrack(blink::WebMediaPlayer::TrackId) override;
  blink::WebMediaPlayer::TrackId AddVideoTrack(const blink::WebString& id,
                                               VideoTrackKind,
                                               const blink::WebString& label,
                                               const blink::WebString& language,
                                               bool selected) override;
  void RemoveVideoTrack(blink::WebMediaPlayer::TrackId) override;
  void AddTextTrack(blink::WebInbandTextTrack*) override;
  void RemoveTextTrack(blink::WebInbandTextTrack*) override;
  void MediaSourceOpened(blink::WebMediaSource*) override;
  void RequestSeek(double) override;
  void RemoteRouteAvailabilityChanged(
      blink::WebRemotePlaybackAvailability) override;
  void ConnectedToRemoteDevice() override;
  void DisconnectedFromRemoteDevice() override;
  void CancelledRemotePlaybackRequest() override;
  void RemotePlaybackStarted() override;
  void RemotePlaybackCompatibilityChanged(const blink::WebURL&,
                                          bool is_compatible) override;
  void OnBecamePersistentVideo(bool) override;
  void ActivateViewportIntersectionMonitoring(bool) override;
  bool IsAutoplayingMuted() override;
  bool HasSelectedVideoTrack() override;
  blink::WebMediaPlayer::TrackId GetSelectedVideoTrackId() override;
  void MediaRemotingStarted(
      const blink::WebString& remote_device_friendly_name) override;
  void MediaRemotingStopped(blink::WebLocalizedString::Name error_msg) override;
  void PictureInPictureStarted() override;
  void PictureInPictureStopped() override;
  bool HasNativeControls() override;
  bool IsAudioElement() override;
  blink::WebMediaPlayer::DisplayType DisplayType() const override;
  blink::WebRemotePlaybackClient* RemotePlaybackClient() override;
  gfx::ColorSpace TargetColorSpace() override;

 private:
  blink::WebMediaPlayerClient* client_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_CONTENT_H_
