// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_VIDEO_DECODER_SERVICE_H_
#define MEDIA_MOJO_SERVICES_MOJO_VIDEO_DECODER_SERVICE_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "media/base/decode_status.h"
#include "media/base/overlay_info.h"
#include "media/base/video_decoder.h"
#include "media/mojo/interfaces/video_decoder.mojom.h"
#include "media/mojo/services/mojo_media_client.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace media {

class ContentDecryptionModule;
class DecoderBuffer;
class MojoCdmServiceContext;
class MojoDecoderBufferReader;
class MojoMediaClient;
class MojoMediaLog;
class MojoVideoFrameHandleReleaserService;
class VideoFrame;

// Implementation of a mojom::VideoDecoder which runs in the GPU process,
// and wraps a media::VideoDecoder.
class MojoVideoDecoderService final : public mojom::VideoDecoder {
 public:
  explicit MojoVideoDecoderService(
      MojoMediaClient* mojo_media_client,
      MojoCdmServiceContext* mojo_cdm_service_context,
      service_manager::ServiceContextRefFactory* context_ref_factory);
  ~MojoVideoDecoderService() final;

  // mojom::VideoDecoder implementation
  void Construct(mojom::VideoDecoderClientAssociatedPtrInfo client,
                 mojom::MediaLogAssociatedPtrInfo media_log,
                 mojom::VideoFrameHandleReleaserAssociatedRequest
                     video_frame_handle_releaser,
                 mojo::ScopedDataPipeConsumerHandle decoder_buffer_pipe,
                 mojom::CommandBufferIdPtr command_buffer_id) final;
  void Initialize(const VideoDecoderConfig& config,
                  bool low_delay,
                  int32_t cdm_id,
                  InitializeCallback callback) final;
  void Decode(mojom::DecoderBufferPtr buffer, DecodeCallback callback) final;
  void Reset(ResetCallback callback) final;
  void OnOverlayInfoChanged(const OverlayInfo& overlay_info) final;

 private:
  // Helper methods so that we can bind them with a weak pointer to avoid
  // running mojom::VideoDecoder callbacks after connection error happens and
  // |this| is deleted. It's not safe to run the callbacks after a connection
  // error.
  void OnDecoderInitialized(InitializeCallback callback,
                            scoped_refptr<ContentDecryptionModule> cdm,
                            bool success);
  void OnDecoderRead(DecodeCallback callback,
                     scoped_refptr<DecoderBuffer> buffer);
  void OnDecoderDecoded(DecodeCallback callback, DecodeStatus status);
  void OnDecoderReset(ResetCallback callback);
  void OnDecoderOutput(const scoped_refptr<VideoFrame>& frame);

  void OnDecoderRequestedOverlayInfo(
      bool restart_for_transitions,
      const ProvideOverlayInfoCB& provide_overlay_info_cb);

  MojoMediaClient* mojo_media_client_;
  service_manager::ServiceContextRefFactory* context_ref_factory_;

  mojom::VideoDecoderClientAssociatedPtr client_;
  std::unique_ptr<MojoMediaLog> media_log_;

  // Owned by |this| until |this| is destructed, at which point we call
  // Destroy() to pass ownership to the MojoVideoFrameHandleReleaserService.
  MojoVideoFrameHandleReleaserService*
      mojo_video_frame_handle_releaser_service_ = nullptr;

  std::unique_ptr<MojoDecoderBufferReader> mojo_decoder_buffer_reader_;
  std::unique_ptr<media::VideoDecoder> decoder_;

  ProvideOverlayInfoCB provide_overlay_info_cb_;

  // A helper object required to get CDM from CDM id.
  MojoCdmServiceContext* const mojo_cdm_service_context_ = nullptr;

  // Hold a reference to the CDM to keep it alive for the lifetime of the
  // |decoder_|. The |cdm_| owns the CdmContext which is passed to |decoder_|.
  scoped_refptr<ContentDecryptionModule> cdm_;

  base::WeakPtr<MojoVideoDecoderService> weak_this_;
  base::WeakPtrFactory<MojoVideoDecoderService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoDecoderService);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_VIDEO_DECODER_SERVICE_H_
