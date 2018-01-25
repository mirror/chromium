// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/passthrough_media_player.h"

#include "third_party/WebKit/public/platform/WebSize.h"

using blink::WebAudioSourceProvider;
using blink::WebCanvas;
using blink::WebContentDecryptionModule;
using blink::WebContentDecryptionModuleResult;
using blink::WebMediaPlayer;
using blink::WebMediaPlayerSource;
using blink::WebSecurityOrigin;
using blink::WebSetSinkIdCallbacks;
using blink::WebSize;
using blink::WebString;
using blink::WebTimeRanges;
using blink::WebRect;
using blink::WebURL;

namespace content {

PassthroughMediaPlayer::PassthroughMediaPlayer() {}
PassthroughMediaPlayer::~PassthroughMediaPlayer() {}

void PassthroughMediaPlayer::SetMediaPlayer(
    std::unique_ptr<WebMediaPlayer> player) {
  player_ = std::move(player);
}

void PassthroughMediaPlayer::Load(LoadType load_type,
                                  const WebMediaPlayerSource& source,
                                  CORSMode cors_mode) {
  player_->Load(load_type, source, cors_mode);
}

// Playback controls.
void PassthroughMediaPlayer::Play() {
  player_->Play();
}

void PassthroughMediaPlayer::Pause() {
  player_->Pause();
}

void PassthroughMediaPlayer::Seek(double seconds) {
  player_->Seek(seconds);
}

void PassthroughMediaPlayer::SetRate(double rate) {
  player_->SetRate(rate);
}

void PassthroughMediaPlayer::SetVolume(double volume) {
  player_->SetVolume(volume);
}

void PassthroughMediaPlayer::PictureInPicture() {
  player_->PictureInPicture();
}

void PassthroughMediaPlayer::RequestRemotePlayback() {
  player_->RequestRemotePlayback();
}

void PassthroughMediaPlayer::RequestRemotePlaybackControl() {
  player_->RequestRemotePlaybackControl();
}

void PassthroughMediaPlayer::RequestRemotePlaybackStop() {
  player_->RequestRemotePlaybackStop();
}

void PassthroughMediaPlayer::RequestRemotePlaybackDisabled(bool disabled) {
  player_->RequestRemotePlaybackDisabled(disabled);
}

void PassthroughMediaPlayer::SetPreload(Preload preload) {
  player_->SetPreload(preload);
}

WebTimeRanges PassthroughMediaPlayer::Buffered() const {
  return player_->Buffered();
}

WebTimeRanges PassthroughMediaPlayer::Seekable() const {
  return player_->Seekable();
}

void PassthroughMediaPlayer::SetSinkId(const WebString& sink_id,
                                       const WebSecurityOrigin& origin,
                                       WebSetSinkIdCallbacks* callbacks) {
  return player_->SetSinkId(sink_id, origin, callbacks);
}

// True if the loaded media has a playable video/audio track.
bool PassthroughMediaPlayer::HasVideo() const {
  return player_->HasVideo();
}

bool PassthroughMediaPlayer::HasAudio() const {
  return player_->HasAudio();
}

bool PassthroughMediaPlayer::IsRemote() const {
  return player_->IsRemote();
}

// Dimension of the video.
WebSize PassthroughMediaPlayer::NaturalSize() const {
  return player_->NaturalSize();
}

WebSize PassthroughMediaPlayer::VisibleRect() const {
  return player_->VisibleRect();
}

bool PassthroughMediaPlayer::Paused() const {
  return player_->Paused();
}

bool PassthroughMediaPlayer::Seeking() const {
  return player_->Seeking();
}

double PassthroughMediaPlayer::Duration() const {
  return player_->Duration();
}

double PassthroughMediaPlayer::CurrentTime() const {
  return player_->CurrentTime();
}

WebMediaPlayer::NetworkState PassthroughMediaPlayer::GetNetworkState() const {
  return player_->GetNetworkState();
}

WebMediaPlayer::ReadyState PassthroughMediaPlayer::GetReadyState() const {
  return player_->GetReadyState();
}

WebString PassthroughMediaPlayer::GetErrorMessage() const {
  return player_->GetErrorMessage();
}

bool PassthroughMediaPlayer::DidLoadingProgress() {
  return player_->DidLoadingProgress();
}

bool PassthroughMediaPlayer::HasSingleSecurityOrigin() const {
  return player_->HasSingleSecurityOrigin();
}

bool PassthroughMediaPlayer::DidPassCORSAccessCheck() const {
  return player_->DidPassCORSAccessCheck();
}

double PassthroughMediaPlayer::MediaTimeForTimeValue(double time_value) const {
  return player_->MediaTimeForTimeValue(time_value);
}

unsigned PassthroughMediaPlayer::DecodedFrameCount() const {
  return player_->DecodedFrameCount();
}

unsigned PassthroughMediaPlayer::DroppedFrameCount() const {
  return player_->DroppedFrameCount();
}

unsigned PassthroughMediaPlayer::CorruptedFrameCount() const {
  return player_->CorruptedFrameCount();
}

size_t PassthroughMediaPlayer::AudioDecodedByteCount() const {
  return player_->AudioDecodedByteCount();
}

size_t PassthroughMediaPlayer::VideoDecodedByteCount() const {
  return player_->VideoDecodedByteCount();
}

void PassthroughMediaPlayer::Paint(WebCanvas* canvas,
                                   const WebRect& rect,
                                   cc::PaintFlags& paint_flags,
                                   int already_uploaded_id,
                                   VideoFrameUploadMetadata* out_metadata) {
  player_->Paint(canvas, rect, paint_flags, already_uploaded_id, out_metadata);
}

bool PassthroughMediaPlayer::CopyVideoTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* gl,
    unsigned target,
    unsigned texture,
    unsigned internal_format,
    unsigned format,
    unsigned type,
    int level,
    bool premultiply_alpha,
    bool flip_y,
    int already_uploaded_id,
    VideoFrameUploadMetadata* out_metadata) {
  return player_->CopyVideoTextureToPlatformTexture(
      gl, target, texture, internal_format, format, type, level,
      premultiply_alpha, flip_y, already_uploaded_id, out_metadata);
}

bool PassthroughMediaPlayer::CopyVideoSubTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* gl,
    unsigned target,
    unsigned texture,
    int level,
    int xoffset,
    int yoffset,
    bool premultiply_alpha,
    bool flip_y) {
  return player_->CopyVideoSubTextureToPlatformTexture(
      gl, target, texture, level, xoffset, yoffset, premultiply_alpha, flip_y);
}

bool PassthroughMediaPlayer::TexImageImpl(TexImageFunctionID function_id,
                                          unsigned target,
                                          gpu::gles2::GLES2Interface* gl,
                                          unsigned texture,
                                          int level,
                                          int internalformat,
                                          unsigned format,
                                          unsigned type,
                                          int xoffset,
                                          int yoffset,
                                          int zoffset,
                                          bool flip_y,
                                          bool premultiply_alpha) {
  return player_->TexImageImpl(function_id, target, gl, texture, level,
                               internalformat, format, type, xoffset, yoffset,
                               zoffset, flip_y, premultiply_alpha);
}

WebAudioSourceProvider* PassthroughMediaPlayer::GetAudioSourceProvider() {
  return player_->GetAudioSourceProvider();
}

void PassthroughMediaPlayer::SetContentDecryptionModule(
    WebContentDecryptionModule* cdm,
    WebContentDecryptionModuleResult result) {
  player_->SetContentDecryptionModule(cdm, std::move(result));
}

void PassthroughMediaPlayer::SetPoster(const WebURL& poster) {
  player_->SetPoster(poster);
}

bool PassthroughMediaPlayer::SupportsOverlayFullscreenVideo() {
  return player_->SupportsOverlayFullscreenVideo();
}

void PassthroughMediaPlayer::EnteredFullscreen() {
  player_->EnteredFullscreen();
}

void PassthroughMediaPlayer::ExitedFullscreen() {
  player_->ExitedFullscreen();
}

void PassthroughMediaPlayer::BecameDominantVisibleContent(bool is_dominant) {
  player_->BecameDominantVisibleContent(is_dominant);
}

void PassthroughMediaPlayer::SetIsEffectivelyFullscreen(bool is_fullscreen) {
  player_->SetIsEffectivelyFullscreen(is_fullscreen);
}

void PassthroughMediaPlayer::EnabledAudioTracksChanged(
    const blink::WebVector<TrackId>& enabled_track_ids) {
  player_->EnabledAudioTracksChanged(enabled_track_ids);
}

void PassthroughMediaPlayer::SelectedVideoTrackChanged(
    TrackId* selected_track_id) {
  player_->SelectedVideoTrackChanged(selected_track_id);
}

void PassthroughMediaPlayer::OnHasNativeControlsChanged(bool changed) {
  player_->OnHasNativeControlsChanged(changed);
}

void PassthroughMediaPlayer::OnDisplayTypeChanged(DisplayType display_type) {
  player_->OnDisplayTypeChanged(display_type);
}

}  // namespace content
