// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/mirror_service_proxy.h"

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/media/mirror_service_audio_capture_host.cc"
#include "chrome/browser/media/mirror_service_proxy_factory.h"
#include "chrome/browser/media/mirror_service_video_capture_host.cc"
#include "chrome/browser/media/udp_transport_host_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/service_manager_connection.h"
#include "media/audio/audio_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/limits.h"
#include "media/cast/net/udp_transport.h"
#include "net/log/net_log.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/service_manager/public/cpp/connector.h"

using content::BrowserThread;

namespace media {

namespace {

const int kMinScreenCastDimension = 1;
// Use kMaxDimension/2 as maximum to ensure selected resolutions have area less
// than media::limits::kMaxCanvas.
const int kMaxScreenCastDimension = media::limits::kMaxDimension / 2;
static_assert(kMaxScreenCastDimension * kMaxScreenCastDimension <
                  media::limits::kMaxCanvas,
              "Invalid kMaxScreenCastDimension");

int ClampToValidDimension(int value) {
  if (value > kMaxScreenCastDimension)
    return kMaxScreenCastDimension;
  else if (value < kMinScreenCastDimension)
    return kMinScreenCastDimension;
  return value;
}

content::MediaStreamType ConvertToMediaStreamType(mojom::CaptureType type,
                                                  bool is_audio) {
  switch (type) {
    case mojom::CaptureType::DESKTOP:
      return is_audio ? content::MediaStreamType::MEDIA_DESKTOP_AUDIO_CAPTURE
                      : content::MediaStreamType::MEDIA_DESKTOP_VIDEO_CAPTURE;
    case mojom::CaptureType::TAB:
    case mojom::CaptureType::OFFSCREEN_TAB:
      return is_audio ? content::MediaStreamType::MEDIA_TAB_AUDIO_CAPTURE
                      : content::MediaStreamType::MEDIA_TAB_VIDEO_CAPTURE;
  }
}

VideoCaptureParams ConvertToVideoCaptureParams(mojom::CaptureParam mojo_param) {
  DCHECK_GT(mojo_param.max_frame_rate, 0);
  VideoCaptureParams params;
  const int default_height = ClampToValidDimension(mojo_param.max_height);
  const int default_width = ClampToValidDimension(mojo_param.max_width);
  params.requested_format =
      VideoCaptureFormat(gfx::Size(default_width, default_height),
                         mojo_param.max_frame_rate, PIXEL_FORMAT_I420);
  params.resolution_change_policy =
      mojo_param.type == mojom::CaptureType::DESKTOP
          ? RESOLUTION_POLICY_ANY_WITHIN_LIMIT
          : RESOLUTION_POLICY_FIXED_RESOLUTION;
  if (mojo_param.max_height < kMaxScreenCastDimension &&
      mojo_param.max_width < kMaxScreenCastDimension &&
      mojo_param.min_height > kMinScreenCastDimension &&
      mojo_param.min_width > kMinScreenCastDimension) {
    if (mojo_param.max_height == mojo_param.min_height &&
        mojo_param.min_width == mojo_param.min_width) {
      params.resolution_change_policy = RESOLUTION_POLICY_FIXED_RESOLUTION;
    } else if ((100 * mojo_param.min_width / mojo_param.min_height) ==
               (100 * mojo_param.max_width / mojo_param.max_height)) {
      params.resolution_change_policy = RESOLUTION_POLICY_FIXED_ASPECT_RATIO;
    } else {
      params.resolution_change_policy = RESOLUTION_POLICY_ANY_WITHIN_LIMIT;
    }
  }
  DCHECK(params.IsValid());

  return params;
}

}  // namespace

MirrorServiceProxy::MirrorServiceProxy()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      service_binding_(this),
      client_binding_(this),
      weak_factory_(this) {
  DVLOG(1) << __func__;
}

MirrorServiceProxy::~MirrorServiceProxy() {
  DVLOG(1) << __func__;
}

// static
void MirrorServiceProxy::BindToRequest(content::BrowserContext* context,
                                       mojom::MirrorServiceProxyRequest request,
                                       content::RenderFrameHost* source) {
  MirrorServiceProxy* impl = static_cast<MirrorServiceProxy*>(
      MirrorServiceProxyFactory::GetApiForBrowserContext(context));
  DCHECK(impl);
  impl->BindToMojoRequest(std::move(request), context);
}

void MirrorServiceProxy::BindToMojoRequest(
    mojom::MirrorServiceProxyRequest request,
    content::BrowserContext* context) {
  DVLOG(1) << __func__;
  service_binding_.Bind(std::move(request));
  service_binding_.set_connection_error_handler(
      base::BindOnce(&MirrorServiceProxy::OnRemoteMirrorServiceConnectionError,
                     base::Unretained(this)));
  DCHECK(context);
  Profile* profile = Profile::FromBrowserContext(context);
  Browser* target_browser = chrome::FindAnyBrowser(profile, false);
  if (target_browser) {
    content::WebContents* target_contents =
        target_browser->tab_strip_model()->GetActiveWebContents();
    if (target_contents) {
      content::RenderFrameHost* const main_frame =
          target_contents->GetMainFrame();
      target_process_id_ = main_frame->GetProcess()->GetID();
      target_frame_id_ = main_frame->GetRoutingID();
      DVLOG(1) << "target process id: " << target_process_id_
               << "target frame id: " << target_frame_id_;
    }
  }
  url_request_context_getter_ = profile->GetRequestContext();
}

void MirrorServiceProxy::ConnectToMirrorService() {
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface("mirror", &mirror_service_);
  DCHECK(mirror_service_);
  mirror_service_.set_connection_error_handler(
      base::BindOnce(&MirrorServiceProxy::OnMirrorServiceConnectionError,
                     base::Unretained(this)));
}

void MirrorServiceProxy::OnMirrorServiceConnectionError() {
  DVLOG(1) << __func__;
  mirror_service_ = nullptr;
}

void MirrorServiceProxy::OnRemoteMirrorServiceConnectionError() {
  DVLOG(1) << __func__;
  mirror_service_ = nullptr;
}

void MirrorServiceProxy::GetSupportedSenderConfigs(
    bool has_audio,
    bool has_video,
    mojom::MirrorServiceProxy::GetSupportedSenderConfigsCallback callback) {
  DVLOG(1) << __func__;

  if (!mirror_service_)
    ConnectToMirrorService();
  mirror_service_->GetSupportedSenderConfigs(has_audio, has_video,
                                             std::move(callback));
}

void MirrorServiceProxy::Start(
    mojom::CaptureParamPtr capture_param,
    mojom::UdpOptionsPtr udp_param,
    mojom::SenderConfigPtr audio_config,
    mojom::SenderConfigPtr video_config,
    mojom::StreamingParamsPtr streaming_param,
    mojom::MirrorClientPtr client,
    mojom::MirrorServiceProxy::StartCallback callback) {
  DVLOG(1) << __func__;
  client_ = std::move(client);
  // The connection was lost before mirror start.
  if (!mirror_service_) {
    std::move(callback).Run(false);
    return;
  }

  // Required for mirroring.
  DCHECK(capture_param);
  has_video_ = capture_param->has_video;
  has_audio_ = capture_param->has_audio;
  audio_stream_type_ = ConvertToMediaStreamType(capture_param->type, true);
  video_stream_type_ = ConvertToMediaStreamType(capture_param->type, false);
  device_id_ = base::StringPrintf(
      "web-contents-media-stream://%i:%i%s", target_process_id_,
      target_frame_id_,
      capture_param->enable_auto_throttling ? "?throttling=auto" : "");

  // Setup UDP transport on io thread.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &MirrorServiceProxy::CreateUdpTransportHostOnIO,
          base::Unretained(this), std::move(udp_param),
          weak_factory_.GetWeakPtr(),
          BindToCurrentLoop(base::BindOnce(
              &MirrorServiceProxy::OnUdpTransportHostCreated,
              weak_factory_.GetWeakPtr(),
              ConvertToVideoCaptureParams(*capture_param),
              std::move(audio_config), std::move(video_config),
              std::move(streaming_param), base::Passed(&callback)))));
}

void MirrorServiceProxy::CreateUdpTransportHostOnIO(
    mojom::UdpOptionsPtr udp_param,
    base::WeakPtr<MirrorServiceProxy> weak_service,
    base::OnceCallback<void(mojom::UdpTransportHostPtr)> callback) {
  net::IPAddress ip;
  if (!ip.AssignFromIPLiteral(udp_param->receiver_ip_address)) {
    DVLOG(1) << "pass the ip address error: " << udp_param->receiver_ip_address;
    base::ResetAndReturn(&callback).Run(nullptr);
  }
  net::IPEndPoint ip_endpoint(ip, udp_param->receiver_port);
  std::unique_ptr<cast::UdpTransport> udp_transport(new cast::UdpTransport(
      url_request_context_getter_->GetURLRequestContext()->net_log(),
      base::ThreadTaskRunnerHandle::Get(), net::IPEndPoint(), ip_endpoint,
      base::Bind(&MirrorServiceProxy::OnTransportStatusChanged, weak_service)));

  base::DictionaryValue options;
  if (udp_param->dscp_enabled) {
    DCHECK(options.SetBoolean("DSCP", true));
    udp_transport->SetUdpOptions(options);
  }

  UdpTransportHostImpl::Create(std::move(udp_transport), std::move(callback));
}

void MirrorServiceProxy::OnUdpTransportHostCreated(
    const VideoCaptureParams& capture_param,
    mojom::SenderConfigPtr audio_config,
    mojom::SenderConfigPtr video_config,
    mojom::StreamingParamsPtr streaming_param,
    mojom::MirrorServiceProxy::StartCallback callback,
    mojom::UdpTransportHostPtr udp_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!udp_host) {
    DVLOG(1) << "Failed to create udp transport.";
    std::move(callback).Run(false);
    return;
  }

  audio_thread_ = base::MakeUnique<AudioThreadImpl>();

  // Create video capture host.
  if (has_video_) {
    DVLOG(1) << "Create Video Capture host.";
    base::OnceCallback<void(content::mojom::VideoCaptureHostPtr)>
        video_host_create_cb = BindToCurrentLoop(
            base::BindOnce(&MirrorServiceProxy::OnVideoCaptureHostCreated,
                           weak_factory_.GetWeakPtr(), std::move(audio_config),
                           std::move(video_config), std::move(streaming_param),
                           base::Passed(&callback), std::move(udp_host)));
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner =
        audio_thread_->GetTaskRunner();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&MirrorServiceVideoCaptureHost::Create, device_id_,
                       video_stream_type_, capture_param, audio_task_runner,
                       base::Passed(&video_host_create_cb)));
  } else {
    DVLOG(1) << "No video.";
    OnVideoCaptureHostCreated(std::move(audio_config), std::move(video_config),
                              std::move(streaming_param), std::move(callback),
                              std::move(udp_host),
                              content::mojom::VideoCaptureHostPtr());
  }
}

void MirrorServiceProxy::OnVideoCaptureHostCreated(
    mojom::SenderConfigPtr audio_config,
    mojom::SenderConfigPtr video_config,
    mojom::StreamingParamsPtr streaming_param,
    mojom::MirrorServiceProxy::StartCallback callback,
    mojom::UdpTransportHostPtr udp_host,
    content::mojom::VideoCaptureHostPtr video_capture_host) {
  DVLOG(1) << __func__;
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!mirror_service_) {
    DVLOG(1) << "service stopped before start streaming.";
    std::move(callback).Run(false);
  }

  if (has_audio_) {
    MirrorServiceAudioCaptureHost::AudioCaptureHostCreatedCallback
        audio_host_create_cb = BindToCurrentLoop(
            base::BindOnce(&MirrorServiceProxy::OnAudioCaptureHostCreated,
                           weak_factory_.GetWeakPtr(), std::move(audio_config),
                           std::move(video_config), std::move(streaming_param),
                           base::Passed(&callback), std::move(udp_host),
                           std::move(video_capture_host)));
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner =
        audio_thread_->GetTaskRunner();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&MirrorServiceAudioCaptureHost::Create, device_id_,
                       audio_stream_type_, base::Passed(&audio_host_create_cb),
                       audio_task_runner));
  } else {
    DVLOG(1) << "No audio.";
    OnAudioCaptureHostCreated(std::move(audio_config), std::move(video_config),
                              std::move(streaming_param), std::move(callback),
                              std::move(udp_host),
                              std::move(video_capture_host), AudioParameters(),
                              mojom::AudioCaptureHostPtr());
  }
}

void MirrorServiceProxy::OnAudioCaptureHostCreated(
    mojom::SenderConfigPtr audio_config,
    mojom::SenderConfigPtr video_config,
    mojom::StreamingParamsPtr streaming_param,
    mojom::MirrorServiceProxy::StartCallback callback,
    mojom::UdpTransportHostPtr udp_host,
    content::mojom::VideoCaptureHostPtr video_capture_host,
    const AudioParameters& params,
    mojom::AudioCaptureHostPtr audio_capture_host) {
  DVLOG(1) << __func__;
  mojom::MirrorClientPtr ptr;
  client_binding_.Bind(mojo::MakeRequest(&ptr));
  mirror_service_->Start(
      std::move(audio_config), std::move(video_config),
      std::move(streaming_param), std::move(ptr), std::move(audio_capture_host),
      std::move(video_capture_host), params, std::move(udp_host),
      BindToCurrentLoop(base::Bind(&MirrorServiceProxy::OnStreamingStarted,
                                   weak_factory_.GetWeakPtr(),
                                   base::Passed(&callback))));
}

void MirrorServiceProxy::OnStreamingStarted(
    mojom::MirrorServiceProxy::StartCallback callback,
    bool success) {
  DVLOG(1) << __func__ << "streaming started: " << success;
  std::move(callback).Run(success);
}

void MirrorServiceProxy::Stop() {
  DVLOG(1) << __func__;
  if (mirror_service_) {
    mirror_service_->Stop();
  }
}

void MirrorServiceProxy::OnStopped() {
  DVLOG(1) << __func__;

  mirror_service_ = nullptr;
  if (client_) {
    client_->OnStopped();
    client_ = nullptr;
  }
}

void MirrorServiceProxy::OnError(mojom::MirrorError error) {
  DVLOG(1) << __func__;

  if (client_)
    client_->OnError(error);
}

void MirrorServiceProxy::OnTransportStatusChanged(
    cast::CastTransportStatus status) {
  if (status != media::cast::TRANSPORT_SOCKET_ERROR) {
    DVLOG(1) << "Warning: Unexpected status= " << status;
    return;
  }
  DVLOG(1) << "Socket error.";
  OnError(mojom::MirrorError::CAST_TRANSPORT_ERROR);
}

}  // namespace media
