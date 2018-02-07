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
#include "media/capture/video_capture_types.h"
#include "media/cast/net/udp_transport_impl.h"
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
          ? ResolutionChangePolicy::ANY_WITHIN_LIMIT
          : ResolutionChangePolicy::FIXED_RESOLUTION;
  if (mojo_param.max_height < kMaxScreenCastDimension &&
      mojo_param.max_width < kMaxScreenCastDimension &&
      mojo_param.min_height > kMinScreenCastDimension &&
      mojo_param.min_width > kMinScreenCastDimension) {
    if (mojo_param.max_height == mojo_param.min_height &&
        mojo_param.min_width == mojo_param.min_width) {
      params.resolution_change_policy =
          ResolutionChangePolicy::FIXED_RESOLUTION;
    } else if ((100 * mojo_param.min_width / mojo_param.min_height) ==
               (100 * mojo_param.max_width / mojo_param.max_height)) {
      params.resolution_change_policy =
          ResolutionChangePolicy::FIXED_ASPECT_RATIO;
    } else {
      params.resolution_change_policy =
          ResolutionChangePolicy::ANY_WITHIN_LIMIT;
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
  browser_context_ = context;
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

  Profile* profile = Profile::FromBrowserContext(browser_context_);
  Browser* target_browser = chrome::FindAnyBrowser(profile, false);
  int target_process_id = -1;
  int target_frame_id = -1;
  if (target_browser) {
    content::WebContents* target_contents =
        target_browser->tab_strip_model()->GetActiveWebContents();
    if (target_contents) {
      content::RenderFrameHost* const main_frame =
          target_contents->GetMainFrame();
      target_process_id = main_frame->GetProcess()->GetID();
      target_frame_id = main_frame->GetRoutingID();
      DVLOG(1) << "target process id: " << target_process_id
               << "target frame id: " << target_frame_id;
    }
  }

  if (target_process_id == -1 || target_frame_id == -1) {
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
      "web-contents-media-stream://%i:%i%s", target_process_id, target_frame_id,
      capture_param->enable_auto_throttling ? "?throttling=auto" : "");

  audio_thread_ = base::MakeUnique<AudioThreadImpl>();

  // Create video capture host.
  if (has_video_) {
    DVLOG(1) << "Create Video Capture host.";
    base::OnceCallback<void(mojom::VideoCaptureHostPtr)> video_host_create_cb =
        BindToCurrentLoop(
            base::BindOnce(&MirrorServiceProxy::OnVideoCaptureHostCreated,
                           weak_factory_.GetWeakPtr(), std::move(audio_config),
                           std::move(video_config), std::move(udp_param),
                           std::move(streaming_param), std::move(callback)));
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner =
        audio_thread_->GetTaskRunner();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&MirrorServiceVideoCaptureHost::Create, device_id_,
                       video_stream_type_,
                       ConvertToVideoCaptureParams(*capture_param),
                       audio_task_runner, std::move(video_host_create_cb)));
  } else {
    DVLOG(1) << "No video.";
    OnVideoCaptureHostCreated(std::move(audio_config), std::move(video_config),
                              std::move(udp_param), std::move(streaming_param),
                              std::move(callback),
                              mojom::VideoCaptureHostPtr());
  }
}

void MirrorServiceProxy::OnVideoCaptureHostCreated(
    mojom::SenderConfigPtr audio_config,
    mojom::SenderConfigPtr video_config,
    mojom::UdpOptionsPtr udp_param,
    mojom::StreamingParamsPtr streaming_param,
    mojom::MirrorServiceProxy::StartCallback callback,
    mojom::VideoCaptureHostPtr video_capture_host) {
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
                           std::move(video_config), std::move(udp_param),
                           std::move(streaming_param), base::Passed(&callback),
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
                              std::move(udp_param), std::move(streaming_param),
                              std::move(callback),
                              std::move(video_capture_host), AudioParameters(),
                              mojom::AudioCaptureHostPtr());
  }
}

void MirrorServiceProxy::OnAudioCaptureHostCreated(
    mojom::SenderConfigPtr audio_config,
    mojom::SenderConfigPtr video_config,
    mojom::UdpOptionsPtr udp_param,
    mojom::StreamingParamsPtr streaming_param,
    mojom::MirrorServiceProxy::StartCallback callback,
    mojom::VideoCaptureHostPtr video_capture_host,
    const AudioParameters& params,
    mojom::AudioCaptureHostPtr audio_capture_host) {
  DVLOG(1) << __func__;
  mojom::MirrorClientPtr ptr;
  client_binding_.Bind(mojo::MakeRequest(&ptr));

  Profile* profile = Profile::FromBrowserContext(browser_context_);
  network::mojom::NetworkContextPtr network_context =
      profile->CreateMainNetworkContext();
  DCHECK(network_context);

  net::IPAddress ip;
  if (!ip.AssignFromIPLiteral(udp_param->receiver_ip_address)) {
    DVLOG(1) << "pass the ip address error: " << udp_param->receiver_ip_address;
    std::move(callback).Run(false);
  }
  net::IPEndPoint ip_endpoint(ip, udp_param->receiver_port);

  mirror_service_->Start(
      std::move(audio_config), std::move(video_config), ip_endpoint,
      std::move(streaming_param), std::move(ptr), std::move(audio_capture_host),
      std::move(video_capture_host), params, std::move(network_context),
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
