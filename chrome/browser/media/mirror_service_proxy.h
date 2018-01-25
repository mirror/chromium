// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MIRROR_SERVICE_PROXY_H_
#define CHROME_BROWSER_MEDIA_MIRROR_SERVICE_PROXY_H_

#include "base/single_thread_task_runner.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/common/video_capture.mojom.h"
#include "content/public/common/media_stream_request.h"
#include "media/base/audio_parameters.h"
#include "media/capture/video_capture_types.h"
#include "media/cast/net/cast_transport_defines.h"
#include "media/mojo/interfaces/mirror_service.mojom.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class RenderFrameHost;
class BrowserContext;
class VideoCaptureProvider;
class VideoCaptureController;
}  // namespace content

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace media {

class AudioThread;

class MirrorServiceProxy final : public KeyedService,
                                 public mojom::MirrorServiceProxy,
                                 public mojom::MirrorClient {
 public:
  MirrorServiceProxy();
  ~MirrorServiceProxy() override;
  static void BindToRequest(content::BrowserContext* context,
                            mojom::MirrorServiceProxyRequest request,
                            content::RenderFrameHost* source);
  void OnTransportStatusChanged(cast::CastTransportStatus status);
  void OnStreamingStarted(mojom::MirrorServiceProxy::StartCallback callback,
                          bool success);

 private:
  // mojom::MirrorServiceProxy implementation.
  void GetSupportedSenderConfigs(
      bool has_audio,
      bool has_video,
      mojom::MirrorServiceProxy::GetSupportedSenderConfigsCallback callback)
      override;
  void Start(mojom::CaptureParamPtr capture_param,
             mojom::UdpOptionsPtr udp_param,
             mojom::SenderConfigPtr audio_config,
             mojom::SenderConfigPtr video_config,
             mojom::StreamingParamsPtr streaming_param,
             mojom::MirrorClientPtr client,
             mojom::MirrorServiceProxy::StartCallback callback) override;
  void Stop() override;

  // mojom::MirrorClient implementation.
  void OnStopped() override;
  void OnError(mojom::MirrorError error) override;

  void ConnectToMirrorService();
  void OnMirrorServiceConnectionError();
  void OnRemoteMirrorServiceConnectionError();
  void BindToMojoRequest(mojom::MirrorServiceProxyRequest request,
                         content::BrowserContext* context);

  void CreateUdpTransportHostOnIO(
      mojom::UdpOptionsPtr udp_param,
      base::WeakPtr<MirrorServiceProxy> weak_service,
      base::OnceCallback<void(mojom::UdpTransportHostPtr)> callback);

  void OnUdpTransportHostCreated(
      const VideoCaptureParams& capture_param,
      mojom::SenderConfigPtr audio_config,
      mojom::SenderConfigPtr video_config,
      mojom::StreamingParamsPtr streaming_param,
      mojom::MirrorServiceProxy::StartCallback callback,
      mojom::UdpTransportHostPtr udp_host);
  void OnVideoCaptureHostCreated(
      mojom::SenderConfigPtr audio_config,
      mojom::SenderConfigPtr video_config,
      mojom::StreamingParamsPtr streaming_param,
      mojom::MirrorServiceProxy::StartCallback callback,
      mojom::UdpTransportHostPtr udp_host,
      content::mojom::VideoCaptureHostPtr capture_host);
  void OnAudioCaptureHostCreated(
      mojom::SenderConfigPtr audio_config,
      mojom::SenderConfigPtr video_config,
      mojom::StreamingParamsPtr streaming_param,
      mojom::MirrorServiceProxy::StartCallback callback,
      mojom::UdpTransportHostPtr udp_host,
      content::mojom::VideoCaptureHostPtr video_capture_host,
      const AudioParameters& params,
      mojom::AudioCaptureHostPtr audio_capture_host);

  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  mojom::MirrorServiceControllerPtr mirror_service_;
  mojom::MirrorClientPtr client_;

  mojo::Binding<mojom::MirrorServiceProxy> service_binding_;
  mojo::Binding<mojom::MirrorClient> client_binding_;

  int target_process_id_ = -1;
  int target_frame_id_ = -1;
  bool has_video_ = false;
  bool has_audio_ = false;
  content::MediaStreamType video_stream_type_ =
      content::MediaStreamType::MEDIA_NO_SERVICE;
  content::MediaStreamType audio_stream_type_ =
      content::MediaStreamType::MEDIA_NO_SERVICE;
  std::string device_id_;

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  std::unique_ptr<AudioThread> audio_thread_;

  base::WeakPtrFactory<MirrorServiceProxy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MirrorServiceProxy);
};

}  // namespace media

#endif  // CHROME_BROWSER_MEDIA_MIRROR_SERVICE_PROXY_H_
