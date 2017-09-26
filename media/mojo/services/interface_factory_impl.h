// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_INTERFACE_FACTORY_IMPL_H_
#define MEDIA_MOJO_SERVICES_INTERFACE_FACTORY_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "media/mojo/features.h"
#include "media/mojo/interfaces/interface_factory.mojom.h"
#include "media/mojo/services/mojo_cdm_service_context.h"
#include "media/mojo/services/mojo_demuxer_service_context.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace media {

class CdmFactory;
class DemuxerFactory;
class MediaLog;
class MojoMediaClient;
class RendererFactory;

class InterfaceFactoryImpl : public mojom::InterfaceFactory {
 public:
  InterfaceFactoryImpl(
      service_manager::mojom::InterfaceProviderPtr interfaces,
      MediaLog* media_log,
      std::unique_ptr<service_manager::ServiceContextRef> connection_ref,
      MojoMediaClient* mojo_media_client);
  ~InterfaceFactoryImpl() final;

  // mojom::InterfaceFactory implementation.
  void CreateDemuxer(mojom::DemuxerRequest request) final;
  void CreateSourceBuffer(mojom::SourceBufferRequest request) final;
  void CreateAudioDecoder(mojom::AudioDecoderRequest request) final;
  void CreateVideoDecoder(mojom::VideoDecoderRequest request,
                          const std::string& decoder_name) final;
  void CreateRenderer(const std::string& audio_device_id,
                      mojom::RendererRequest request) final;
  void CreateCdm(const std::string& key_system,
                 mojom::ContentDecryptionModuleRequest request) final;

 private:
#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  RendererFactory* GetRendererFactory();
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

#if BUILDFLAG(ENABLE_MOJO_CDM)
  CdmFactory* GetCdmFactory();
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

#if BUILDFLAG(ENABLE_MOJO_DEMUXER)
  DemuxerFactory* GetDemuxerFactory();
#endif  // BUILDFLAG(ENABLE_MOJO_DEMUXER)

  MojoCdmServiceContext cdm_service_context_;

  MojoDemuxerServiceContext demuxer_service_context_;

#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
  mojo::StrongBindingSet<mojom::AudioDecoder> audio_decoder_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
  mojo::StrongBindingSet<mojom::VideoDecoder> video_decoder_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_RENDERER) || BUILDFLAG(ENABLE_MOJO_DEMUXER)
  MediaLog* media_log_;
#endif

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  std::unique_ptr<RendererFactory> renderer_factory_;
  mojo::StrongBindingSet<mojom::Renderer> renderer_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

#if BUILDFLAG(ENABLE_MOJO_CDM) || BUILDFLAG(ENABLE_MOJO_DEMUXER)
  service_manager::mojom::InterfaceProviderPtr interfaces_;
#endif

#if BUILDFLAG(ENABLE_MOJO_CDM)
  std::unique_ptr<CdmFactory> cdm_factory_;
  mojo::StrongBindingSet<mojom::ContentDecryptionModule> cdm_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

#if BUILDFLAG(ENABLE_MOJO_DEMUXER)
  std::unique_ptr<DemuxerFactory> demuxer_factory_;
  mojo::StrongBindingSet<mojom::Demuxer> demuxer_bindings_;
  mojo::StrongBindingSet<mojom::SourceBuffer> source_buffer_bindings_;
#endif  // defined(ENABLE_MOJO_RENDERER)

  std::unique_ptr<service_manager::ServiceContextRef> connection_ref_;
  MojoMediaClient* mojo_media_client_;

  DISALLOW_COPY_AND_ASSIGN(InterfaceFactoryImpl);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_INTERFACE_FACTORY_IMPL_H_
