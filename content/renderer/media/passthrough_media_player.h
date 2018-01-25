// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_H_
#define CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_H_

#include <memory>

#include "third_party/WebKit/public/platform/WebMediaPlayer.h"

namespace content {

class PassthroughMediaPlayer : public blink::WebMediaPlayer {
 public:
  PassthroughMediaPlayer();
  ~PassthroughMediaPlayer() override;

  // Set the underlying player.  Must be called before any of the WMP funcions.
  void SetMediaPlayer(std::unique_ptr<blink::WebMediaPlayer> player);

  void Load(LoadType loat_type,
            const blink::WebMediaPlayerSource& source,
            CORSMode cors_mode) override;
  void Play() override;
  void Pause() override;
  void Seek(double seconds) override;
  void SetRate(double rate) override;
  void SetVolume(double volume) override;
  void PictureInPicture() override;
  void RequestRemotePlayback() override;
  void RequestRemotePlaybackControl() override;
  void RequestRemotePlaybackStop() override;
  void RequestRemotePlaybackDisabled(bool disabled) override;
  void SetPreload(Preload preload) override;
  blink::WebTimeRanges Buffered() const override;
  blink::WebTimeRanges Seekable() const override;
  void SetSinkId(const blink::WebString& sink_id,
                 const blink::WebSecurityOrigin& origin,
                 blink::WebSetSinkIdCallbacks* callbacks) override;
  bool HasVideo() const override;
  bool HasAudio() const override;
  bool IsRemote() const override;
  blink::WebSize NaturalSize() const override;
  blink::WebSize VisibleRect() const override;
  bool Paused() const override;
  bool Seeking() const override;
  double Duration() const override;
  double CurrentTime() const override;
  NetworkState GetNetworkState() const override;
  ReadyState GetReadyState() const override;
  blink::WebString GetErrorMessage() const override;
  bool DidLoadingProgress() override;
  bool HasSingleSecurityOrigin() const override;
  bool DidPassCORSAccessCheck() const override;
  double MediaTimeForTimeValue(double time_value) const override;
  unsigned DecodedFrameCount() const override;
  unsigned DroppedFrameCount() const override;
  unsigned CorruptedFrameCount() const override;
  size_t AudioDecodedByteCount() const override;
  size_t VideoDecodedByteCount() const override;
  void Paint(blink::WebCanvas* canvas,
             const blink::WebRect& rect,
             cc::PaintFlags& paint_flags,
             int already_uploaded_id,
             VideoFrameUploadMetadata* out_metadata) override;
  bool CopyVideoTextureToPlatformTexture(
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
      VideoFrameUploadMetadata* out_metadata) override;
  bool CopyVideoSubTextureToPlatformTexture(gpu::gles2::GLES2Interface* gl,
                                            unsigned target,
                                            unsigned texture,
                                            int level,
                                            int xoffset,
                                            int yoffset,
                                            bool premultiply_alpha,
                                            bool flip_y) override;
  bool TexImageImpl(TexImageFunctionID function_id,
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
                    bool premultiply_alpha) override;
  blink::WebAudioSourceProvider* GetAudioSourceProvider() override;
  void SetContentDecryptionModule(
      blink::WebContentDecryptionModule* cdm,
      blink::WebContentDecryptionModuleResult result) override;
  void SetPoster(const blink::WebURL& poster) override;
  bool SupportsOverlayFullscreenVideo() override;
  void EnteredFullscreen() override;
  void ExitedFullscreen() override;
  void BecameDominantVisibleContent(bool is_dominant) override;
  void SetIsEffectivelyFullscreen(bool is_fullscreen) override;
  void EnabledAudioTracksChanged(
      const blink::WebVector<TrackId>& enabled_track_ids) override;
  void SelectedVideoTrackChanged(TrackId* selected_track_id) override;
  void OnHasNativeControlsChanged(bool changed) override;
  void OnDisplayTypeChanged(DisplayType display_type) override;

 private:
  std::unique_ptr<blink::WebMediaPlayer> player_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_PASSTHROUGH_MEDIA_PLAYER_H_
