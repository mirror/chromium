// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MIRROR_MIRROR_SERVICE_H_
#define MEDIA_MOJO_SERVICES_MIRROR_MIRROR_SERVICE_H_

#include "base/callback.h"
#include "content/common/media/video_capture.h"
#include "content/common/video_capture.mojom.h"
#include "media/base/audio_parameters.h"
#include "media/cast/cast_config.h"
#include "media/cast/constants.h"
#include "media/cast/logging/logging_defines.h"
#include "media/cast/net/cast_transport_defines.h"
#include "media/mojo/interfaces/mirror_service.mojom.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/ui/public/cpp/gpu/gpu.h"

namespace ui {
class Gpu;
}

namespace media {

class VideoCaptureClient;
class AudioInputDevice;
class AudioCaptureCallback;
class UdpTransportClientImpl;

namespace cast {
class CastTransport;
class CastSender;
class CastEnvironment;
}  // namespace cast

class CastRtpStream;

class MEDIA_MOJO_EXPORT MirrorService : public service_manager::Service,
                                        public mojom::MirrorServiceController {
 public:
  static std::unique_ptr<ui::Gpu> gpu_;
  static media::VideoEncodeAccelerator::SupportedProfiles
  GetSupportedVideoEncodeAcceleratorProfiles();
  static scoped_refptr<gpu::GpuChannelHost> EstablishGpuChannelSync(
      bool* connection_error = nullptr);

  MirrorService();
  ~MirrorService() final;
  void OnVideoCaptureStateChanged(content::VideoCaptureState state);
  void CreateVideoEncodeAccelerator(
      const cast::ReceiveVideoEncodeAcceleratorCallback& callback);
  void CreateVideoEncodeMemory(
      size_t size,
      const cast::ReceiveVideoEncodeMemoryCallback& callback);
  void OnReceivedRtpPacket(std::unique_ptr<cast::Packet> packet);
  void OnLoggingEventsReceived(
      std::unique_ptr<std::vector<cast::FrameEvent>> frame_events,
      std::unique_ptr<std::vector<cast::PacketEvent>> packet_events);
  void OnTransportStatusChanged(cast::CastTransportStatus status);
  void OnErrorMessage(const std::string& message);
  void OnOperationalStatusChange(bool is_for_audio,
                                 cast::OperationalStatus status);
  void RequestRefreshFrame();
  void OnAudioData(const media::AudioBus& input_bus,
                   base::TimeTicks estimated_capture_time);

 private:
  // service_manager::Service implementation.
  void OnStart() final;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;
  bool OnServiceManagerConnectionLost() final;

  void Create(mojom::MirrorServiceControllerRequest request);

  // mojom::MirrorServiceController implementation.
  void GetSupportedSenderConfigs(
      bool has_audio,
      bool has_video,
      mojom::MirrorServiceProxy::GetSupportedSenderConfigsCallback callback)
      override;
  void Start(mojom::SenderConfigPtr audio_config,
             mojom::SenderConfigPtr video_config,
             mojom::StreamingParamsPtr streaming_param,
             mojom::MirrorClientPtr client,
             mojom::AudioCaptureHostPtr audio_host,
             content::mojom::VideoCaptureHostPtr video_host,
             const AudioParameters& audio_params,
             mojom::UdpTransportHostPtr udp_host,
             mojom::MirrorServiceProxy::StartCallback callback) override;
  void Stop() override;

  std::unique_ptr<media::VideoEncodeAccelerator> CreateVea();

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<mojom::MirrorServiceController> bindings_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  mojom::MirrorClientPtr client_;

  std::unique_ptr<CastRtpStream> audio_stream_;
  std::unique_ptr<CastRtpStream> video_stream_;
  std::unique_ptr<VideoCaptureClient> video_capture_client_;
  scoped_refptr<AudioInputDevice> audio_capture_client_;
  std::unique_ptr<AudioCaptureCallback> audio_capture_callback_;

  scoped_refptr<cast::CastEnvironment> cast_environment_;
  std::unique_ptr<cast::CastTransport> cast_transport_;
  std::unique_ptr<cast::CastSender> cast_sender_;
  UdpTransportClientImpl* udp_proxy_ = nullptr;  // Own by |cast_transport_|.

  AudioParameters audio_params_;

  media::mojom::VideoEncodeAcceleratorProviderPtr vea_provider_;

  mojo::BindingId binding_id_ = 0;
  bool has_binding_ = false;

  base::WeakPtrFactory<MirrorService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MirrorService);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MIRROR_SERVICE_MIRROR_SERVICE_H_
