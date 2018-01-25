// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/mirror_service.h"

#include "base/callback.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/sync_socket.h"
#include "base/sys_info.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_checker.h"
#include "base/time/default_tick_clock.h"
#include "content/child/child_process.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/utility/utility_thread.h"
#include "media/audio/audio_input_device.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/limits.h"
#include "media/cast/cast_environment.h"
#include "media/cast/cast_sender.h"
#include "media/cast/net/cast_transport.h"
#include "media/cast/net/cast_transport_config.h"
#include "media/gpu/gpu_video_accelerator_util.h"
#include "media/mojo/clients/mojo_video_encode_accelerator.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/mirror/audio_capture_client.h"
#include "media/mojo/services/mirror/cast_rtp_stream.h"
#include "media/mojo/services/mirror/mirror_service_threads.h"
#include "media/mojo/services/mirror/udp_transport_client_impl.h"
#include "media/mojo/services/mirror/video_capture_client.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/ui/public/interfaces/constants.mojom.h"

using media::cast::FrameSenderConfig;
using content::VideoCaptureState;

static base::LazyInstance<MirrorServiceThreads>::DestructorAtExit
    g_mirror_threads = LAZY_INSTANCE_INITIALIZER;

namespace media {

namespace {

constexpr char kCodecNameOpus[] = "OPUS";
constexpr char kCodecNameVp8[] = "VP8";
constexpr char kCodecNameH264[] = "H264";
constexpr char kCodecNameRemoteAudio[] = "REMOTE_AUDIO";
constexpr char kCodecNameRemoteVideo[] = "REMOTE_VIDEO";

// To convert from kilobits per second to bits per second.
constexpr int kBitsPerKilobit = 1000;

// The interval for CastTransport and/or CastRemotingSender to send
// Frame/PacketEvents to renderer process for logging.
constexpr base::TimeDelta kSendEventsInterval = base::TimeDelta::FromSeconds(1);

bool HexDecode(const std::string& input, std::string* output) {
  std::vector<uint8_t> bytes;
  if (!base::HexStringToBytes(input, &bytes))
    return false;
  output->assign(reinterpret_cast<const char*>(&bytes[0]), bytes.size());
  return true;
}

int NumberOfEncodeThreads() {
  // Do not saturate CPU utilization just for encoding. On a lower-end system
  // with only 1 or 2 cores, use only one thread for encoding. On systems with
  // more cores, allow half of the cores to be used for encoding.
  return std::min(8, (base::SysInfo::NumberOfProcessors() + 1) / 2);
}

bool ToFrameSenderConfig(mojom::SenderConfigPtr mojo_config,
                         FrameSenderConfig* config) {
  config->sender_ssrc = mojo_config->sender_ssrc;
  config->receiver_ssrc = mojo_config->receiver_ssrc;
  if (config->sender_ssrc == config->receiver_ssrc) {
    DVLOG(1) << "sender_ssrc " << config->sender_ssrc
             << " cannot be equal to receiver_ssrc";
    return false;
  }
  config->min_playout_delay = base::TimeDelta::FromMilliseconds(
      mojo_config->min_latency_ms ? mojo_config->min_latency_ms
                                  : mojo_config->max_latency_ms);
  config->max_playout_delay =
      base::TimeDelta::FromMilliseconds(mojo_config->max_latency_ms);
  config->animated_playout_delay = base::TimeDelta::FromMilliseconds(
      mojo_config->animated_latency_ms ? mojo_config->animated_latency_ms
                                       : mojo_config->max_latency_ms);
  if (config->min_playout_delay <= base::TimeDelta()) {
    DVLOG(1) << "min_playout_delay " << config->min_playout_delay
             << " must be greater than zero";
    return false;
  }
  if (config->min_playout_delay > config->max_playout_delay) {
    DVLOG(1) << "min_playout_delay " << config->min_playout_delay
             << " must be less than or equal to max_palyout_delay";
    return false;
  }
  if (config->animated_playout_delay < config->min_playout_delay ||
      config->animated_playout_delay > config->max_playout_delay) {
    DVLOG(1) << "animated_playout_delay " << config->animated_playout_delay
             << " must be between (inclusive) the min and max playout delay";
    return false;
  }
  if (mojo_config->codec_name == kCodecNameOpus) {
    config->rtp_payload_type = media::cast::RtpPayloadType::AUDIO_OPUS;
    config->use_external_encoder = false;
    config->rtp_timebase = mojo_config->rtp_timebase
                               ? mojo_config->rtp_timebase
                               : media::cast::kDefaultAudioSamplingRate;
    // Sampling rate must be one of the Opus-supported values.
    switch (config->rtp_timebase) {
      case 48000:
      case 24000:
      case 16000:
      case 12000:
      case 8000:
        break;
      default:
        DVLOG(1) << "rtp_timebase " << config->rtp_timebase << " is invalid";
        return false;
    }
    config->channels = mojo_config->channels ? mojo_config->channels : 2;
    if (config->channels != 1 && config->channels != 2) {
      DVLOG(1) << "channels " << config->channels << " is invalid";
      return false;
    }
    config->min_bitrate = config->start_bitrate = config->max_bitrate =
        mojo_config->max_bitrate ? mojo_config->max_bitrate * kBitsPerKilobit
                                 : media::cast::kDefaultAudioEncoderBitrate;
    config->max_frame_rate = 100;  // 10ms audio frames.
    config->codec = media::cast::CODEC_AUDIO_OPUS;
  } else if (mojo_config->codec_name == kCodecNameVp8 ||
             mojo_config->codec_name == kCodecNameH264) {
    config->rtp_timebase = media::cast::kVideoFrequency;
    config->channels = mojo_config->channels ? mojo_config->channels : 1;
    if (config->channels != 1) {
      DVLOG(1) << "channels " << config->channels << " is invalid";
      return false;
    }
    config->min_bitrate = mojo_config->min_bitrate
                              ? mojo_config->min_bitrate * kBitsPerKilobit
                              : media::cast::kDefaultMinVideoBitrate;
    config->max_bitrate = mojo_config->max_bitrate
                              ? mojo_config->max_bitrate * kBitsPerKilobit
                              : media::cast::kDefaultMaxVideoBitrate;
    if (config->min_bitrate > config->max_bitrate) {
      DVLOG(1) << "min_bitrate " << config->min_bitrate << " is larger than "
               << "max_bitrate " << config->max_bitrate;
      return false;
    }
    config->start_bitrate = config->min_bitrate;
    config->max_frame_rate = std::max(
        1.0, mojo_config->max_frame_rate ? mojo_config->max_frame_rate : 0.0);
    if (config->max_frame_rate > media::limits::kMaxFramesPerSecond) {
      DVLOG(1) << "max_frame_rate " << config->max_frame_rate << " is invalid";
      return false;
    }
    if (mojo_config->codec_name == kCodecNameVp8) {
      config->codec = media::cast::CODEC_VIDEO_VP8;
      config->rtp_payload_type = media::cast::RtpPayloadType::VIDEO_VP8;
      config->use_external_encoder =
          CastRtpStream::IsHardwareVP8EncodingSupported();
    } else {
      config->codec = media::cast::CODEC_VIDEO_H264;
      config->rtp_payload_type = media::cast::RtpPayloadType::VIDEO_H264;
      config->use_external_encoder =
          CastRtpStream::IsHardwareH264EncodingSupported();
    }
    if (!config->use_external_encoder)
      config->video_codec_params.number_of_encode_threads =
          NumberOfEncodeThreads();
  } else if (mojo_config->codec_name == kCodecNameRemoteAudio) {
    config->codec = media::cast::CODEC_AUDIO_REMOTE;
    config->rtp_payload_type = media::cast::RtpPayloadType::REMOTE_AUDIO;
  } else if (mojo_config->codec_name == kCodecNameRemoteVideo) {
    config->codec = media::cast::CODEC_VIDEO_REMOTE;
    config->rtp_payload_type = media::cast::RtpPayloadType::REMOTE_VIDEO;
  } else {
    DVLOG(1) << "codec_name " << mojo_config->codec_name << " is invalid";
    return false;
  }
  if (!mojo_config->aes_key.empty() &&
      !HexDecode(mojo_config->aes_key, &config->aes_key)) {
    DVLOG(1) << "Invalid aes key.";
    return false;
  }
  if (!mojo_config->aes_ivmask.empty() &&
      !HexDecode(mojo_config->aes_ivmask, &config->aes_iv_mask)) {
    DVLOG(1) << "Invalid aes iv mask.";
    return false;
  }
  return true;
}

mojom::SenderConfigPtr FromFrameSenderConfig(const FrameSenderConfig& config) {
  mojom::SenderConfigPtr mojo_config = mojom::SenderConfig::New();
  mojo_config->rtp_payload_type = static_cast<int>(config.rtp_payload_type);
  mojo_config->sender_ssrc = config.sender_ssrc;
  mojo_config->receiver_ssrc = config.receiver_ssrc;
  mojo_config->min_latency_ms = config.min_playout_delay.InMilliseconds();
  mojo_config->max_latency_ms = config.max_playout_delay.InMilliseconds();
  mojo_config->animated_latency_ms =
      config.animated_playout_delay.InMilliseconds();
  switch (config.codec) {
    case media::cast::CODEC_AUDIO_OPUS:
      mojo_config->codec_name = kCodecNameOpus;
      break;
    case media::cast::CODEC_VIDEO_VP8:
      mojo_config->codec_name = kCodecNameVp8;
      break;
    case media::cast::CODEC_VIDEO_H264:
      mojo_config->codec_name = kCodecNameH264;
      break;
    case media::cast::CODEC_AUDIO_REMOTE:
      mojo_config->codec_name = kCodecNameRemoteAudio;
      break;
    case media::cast::CODEC_VIDEO_REMOTE:
      mojo_config->codec_name = kCodecNameRemoteVideo;
      break;
    default:
      NOTREACHED();
  }
  mojo_config->rtp_timebase = config.rtp_timebase;
  mojo_config->min_bitrate = config.min_bitrate;
  mojo_config->max_bitrate = config.max_bitrate;
  mojo_config->channels = config.channels;
  mojo_config->max_frame_rate = config.max_frame_rate;
  return mojo_config;
}

class TransportClient final : public cast::CastTransport::Client {
 public:
  explicit TransportClient(MirrorService* service) : service_(service) {}
  ~TransportClient() override {}

  void OnStatusChanged(cast::CastTransportStatus status) override {
    service_->OnTransportStatusChanged(status);
  }

  void OnLoggingEventsReceived(
      std::unique_ptr<std::vector<cast::FrameEvent>> frame_events,
      std::unique_ptr<std::vector<cast::PacketEvent>> packet_events) override {
    service_->OnLoggingEventsReceived(std::move(frame_events),
                                      std::move(packet_events));
  }

  void ProcessRtpPacket(std::unique_ptr<cast::Packet> packet) override {
    service_->OnReceivedRtpPacket(std::move(packet));
  }

 private:
  MirrorService* service_;

  DISALLOW_COPY_AND_ASSIGN(TransportClient);
};

}  // namespace

std::unique_ptr<ui::Gpu> MirrorService::gpu_ = nullptr;

MirrorService::MirrorService() : weak_factory_(this) {
  // : io_task_runner_(content::ChildThread::Get()->GetIOTaskRunner()) {
  DVLOG(3) << __func__;
  registry_.AddInterface<mojom::MirrorServiceController>(
      base::Bind(&MirrorService::Create, base::Unretained(this)));
}

MirrorService::~MirrorService() {
  DVLOG(1) << __func__;
  // io_task_runner_->DeleteSoon(FROM_HERE, video_capture_client_.release());
  if (video_capture_client_)
    video_capture_client_->Stop();
}

void MirrorService::OnStart() {
  DVLOG(3) << __func__;
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context()))));
}

void MirrorService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DVLOG(3) << __func__;
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool MirrorService::OnServiceManagerConnectionLost() {
  DVLOG(3) << __func__;
  return true;
}

void MirrorService::Create(mojom::MirrorServiceControllerRequest request) {
  binding_id_ = bindings_.AddBinding(this, std::move(request));
  DVLOG(3) << __func__ << " binding_id_=" << binding_id_;
  has_binding_ = true;
  bindings_.set_connection_error_handler(
      base::Bind(&MirrorService::Stop, base::Unretained(this)));
}

void MirrorService::GetSupportedSenderConfigs(
    bool has_audio,
    bool has_video,
    mojom::MirrorServiceProxy::GetSupportedSenderConfigsCallback callback) {
  DVLOG(1) << __func__;
  std::vector<mojom::SenderConfigPtr> audio_configs;
  std::vector<mojom::SenderConfigPtr> video_configs;
  if (has_audio) {
    const std::vector<FrameSenderConfig> configs =
        CastRtpStream::GetSupportedConfigs(true, false);
    for (const FrameSenderConfig& config : configs) {
      audio_configs.push_back(FromFrameSenderConfig(config));
    }
  }
  if (has_video) {
    const std::vector<FrameSenderConfig> configs =
        CastRtpStream::GetSupportedConfigs(false, false);
    for (const FrameSenderConfig& config : configs) {
      video_configs.push_back(FromFrameSenderConfig(config));
    }
  }
  std::move(callback).Run(std::move(audio_configs), std::move(video_configs));
}

void MirrorService::Start(
    mojom::SenderConfigPtr audio_config,
    mojom::SenderConfigPtr video_config,
    mojom::StreamingParamsPtr streaming_param,
    mojom::MirrorClientPtr client,
    mojom::AudioCaptureHostPtr audio_capture_host,
    content::mojom::VideoCaptureHostPtr video_capture_host,
    const AudioParameters& audio_params,
    mojom::UdpTransportHostPtr udp_host,
    mojom::MirrorServiceProxy::StartCallback callback) {
  DVLOG(1) << __func__;

  client_ = std::move(client);
  DCHECK(!video_capture_client_);
  if (video_capture_host) {
    video_capture_client_.reset(new VideoCaptureClient(
        std::move(video_capture_host),
        base::Bind(&MirrorService::OnVideoCaptureStateChanged,
                   weak_factory_.GetWeakPtr())));
  }
  DCHECK(!audio_capture_client_);
  if (audio_capture_host) {
    std::unique_ptr<AudioCaptureIpc> audio_capture_ipc(
        new AudioCaptureIpc(std::move(audio_capture_host)));
    audio_capture_client_ = new AudioInputDevice(
        std::move(audio_capture_ipc), base::ThreadTaskRunnerHandle::Get());
    audio_params_ = audio_params;
    if (!audio_params_.IsValid())
      base::ResetAndReturn(&callback).Run(false);
  }

  DCHECK(!cast_sender_);
  DCHECK(!udp_proxy_);
  DCHECK(!cast_transport_);
  cast_environment_ = new cast::CastEnvironment(
      std::unique_ptr<base::TickClock>(new base::DefaultTickClock()),
      base::ThreadTaskRunnerHandle::Get(),
      g_mirror_threads.Get().GetAudioEncodeTaskRunner(),
      g_mirror_threads.Get().GetVideoEncodeTaskRunner());
  std::unique_ptr<UdpTransportClientImpl> udp_proxy =
      base::MakeUnique<UdpTransportClientImpl>(std::move(udp_host));
  udp_proxy_ = udp_proxy.get();
  cast_transport_ = cast::CastTransport::Create(
      cast_environment_->Clock(), kSendEventsInterval,
      base::MakeUnique<TransportClient>(this), std::move(udp_proxy),
      base::ThreadTaskRunnerHandle::Get());
  cast_sender_ =
      cast::CastSender::Create(cast_environment_, cast_transport_.get());

  DCHECK(!audio_stream_);
  DCHECK(!video_stream_);
  if (audio_config) {
    FrameSenderConfig audio_sender_config;
    if (!ToFrameSenderConfig(std::move(audio_config), &audio_sender_config)) {
      std::move(callback).Run(false);
      return;
    }
    DCHECK_LE(audio_sender_config.rtp_payload_type,
              cast::RtpPayloadType::AUDIO_LAST);
    audio_stream_.reset(new CastRtpStream(true, cast_sender_.get(), this));
    audio_stream_->Start(audio_sender_config);
    audio_stream_->SetAudioParameters(audio_params_);
  }

  if (video_config) {
    DCHECK(!video_stream_);
    FrameSenderConfig video_sender_config;
    if (!ToFrameSenderConfig(std::move(video_config), &video_sender_config)) {
      std::move(callback).Run(false);
      return;
    }
    DCHECK_GT(video_sender_config.rtp_payload_type,
              cast::RtpPayloadType::AUDIO_LAST);
    video_stream_.reset(new CastRtpStream(false, cast_sender_.get(), this));
    video_stream_->Start(video_sender_config);
  }

  std::move(callback).Run(true);
}

void MirrorService::Stop() {
  DVLOG(1) << __func__;
  if (video_stream_) {
    video_stream_->Stop();
    video_stream_.reset();
  }
  if (audio_stream_) {
    audio_stream_->Stop();
    audio_stream_.reset();
  }
  if (video_capture_client_) {
    video_capture_client_->Stop();
    // video_capture_client_.reset();
  }
  if (audio_capture_client_) {
    audio_capture_client_->Stop();
  }
  if (has_binding_) {
    DVLOG(1) << "RemoveBinding: has_binding_=" << has_binding_
             << " binding_id_=" << binding_id_;
    has_binding_ = false;
    bindings_.RemoveBinding(binding_id_);
  }
  if (client_)
    client_->OnStopped();
}

void MirrorService::OnVideoCaptureStateChanged(VideoCaptureState state) {
  DVLOG(1) << __func__ << " state: " << state;
  switch (state) {
    case content::VIDEO_CAPTURE_STATE_STARTED:
    case content::VIDEO_CAPTURE_STATE_STOPPED:
    case content::VIDEO_CAPTURE_STATE_PAUSED:
    case content::VIDEO_CAPTURE_STATE_RESUMED:
    case content::VIDEO_CAPTURE_STATE_ENDED:
      break;
    case content::VIDEO_CAPTURE_STATE_ERROR:
      if (client_)
        client_->OnError(mojom::MirrorError::VIDEO_CAPTURE_ERROR);
      break;
    case content::VIDEO_CAPTURE_STATE_STARTING:
    case content::VIDEO_CAPTURE_STATE_STOPPING:
      break;
  }
}

void MirrorService::OnOperationalStatusChange(bool is_for_audio,
                                              cast::OperationalStatus status) {
  DCHECK(cast_sender_);

  switch (status) {
    case media::cast::STATUS_UNINITIALIZED:
    case media::cast::STATUS_CODEC_REINIT_PENDING:
      // Not an error.
      // TODO(miu): As an optimization, signal the client to pause sending more
      // frames until the state becomes STATUS_INITIALIZED again.
      break;
    case media::cast::STATUS_INITIALIZED:
      // Once initialized, run the "frame input available" callback to allow the
      // client to begin sending frames.  If STATUS_INITIALIZED is encountered
      // again, do nothing since this is only an indication that the codec has
      // successfully re-initialized.
      if (is_for_audio) {
        DVLOG(1) << "start receiving audio data.";
        DCHECK(audio_capture_client_);
        audio_stream_->StartDeliver();
        audio_capture_callback_.reset(new AudioCaptureCallback(
            base::Bind(&MirrorService::OnAudioData, base::Unretained(this)),
            base::Bind(&MirrorService::OnErrorMessage,
                       base::Unretained(this))));
        audio_capture_client_->Initialize(audio_params_,
                                          audio_capture_callback_.get(), 3);
        audio_capture_client_->Start();
      } else {
        DVLOG(1) << "start receiving video frames.";
        DCHECK(video_capture_client_);
        video_stream_->StartDeliver();
        video_capture_client_->Start(BindToCurrentLoop(
            base::Bind(&CastVideoSink::Deliverer::OnVideoFrame,
                       video_stream_->GetVideoDeliverer())));
      }
      break;
    case media::cast::STATUS_INVALID_CONFIGURATION:
      OnErrorMessage(base::StringPrintf("Invalid %s configuration.",
                                        is_for_audio ? "audio" : "video"));
      break;
    case media::cast::STATUS_UNSUPPORTED_CODEC:
      OnErrorMessage(base::StringPrintf("%s codec not supported.",
                                        is_for_audio ? "Audio" : "Video"));
      break;
    case media::cast::STATUS_CODEC_INIT_FAILED:
      OnErrorMessage(base::StringPrintf("%s codec initialization failed.",
                                        is_for_audio ? "Audio" : "Video"));
      break;
    case media::cast::STATUS_CODEC_RUNTIME_ERROR:
      OnErrorMessage(base::StringPrintf("%s codec runtime error.",
                                        is_for_audio ? "Audio" : "Video"));
      break;
  }
}

void MirrorService::OnErrorMessage(const std::string& message) {
  DVLOG(1) << __func__ << ": " << message;
  if (client_)
    client_->OnError(mojom::MirrorError::CAST_TRANSPORT_ERROR);
}

void MirrorService::OnTransportStatusChanged(cast::CastTransportStatus status) {
  DVLOG(1) << __func__ << ": status=" << status;
  switch (status) {
    case media::cast::TRANSPORT_STREAM_UNINITIALIZED:
    case media::cast::TRANSPORT_STREAM_INITIALIZED:
      return;  // Not errors, do nothing.
    case media::cast::TRANSPORT_INVALID_CRYPTO_CONFIG:
      DVLOG(1) << "Warning: unexpected status: "
               << "TRANSPORT_INVALID_CRYPTO_CONFIG";
      client_->OnError(mojom::MirrorError::CAST_TRANSPORT_ERROR);
      break;
    case media::cast::TRANSPORT_SOCKET_ERROR:
      DVLOG(1) << "Warning: unexpected status: "
               << "TRANSPORT_SOCKET_ERROR";
      break;
  }
}

void MirrorService::OnLoggingEventsReceived(
    std::unique_ptr<std::vector<cast::FrameEvent>> frame_events,
    std::unique_ptr<std::vector<cast::PacketEvent>> packet_events) {
  cast_environment_->logger()->DispatchBatchOfEvents(std::move(frame_events),
                                                     std::move(packet_events));
}

void MirrorService::OnReceivedRtpPacket(std::unique_ptr<cast::Packet> packet) {
  // Do nothing.
  DVLOG(1) << __func__;
}

void MirrorService::RequestRefreshFrame() {
  if (video_capture_client_)
    video_capture_client_->RequestRefreshFrame();
}

void MirrorService::OnAudioData(const media::AudioBus& input_bus,
                                base::TimeTicks estimated_capture_time) {
  if (!audio_stream_)
    return;
  audio_stream_->OnAudioData(input_bus, estimated_capture_time);
}

media::VideoEncodeAccelerator::SupportedProfiles
MirrorService::GetSupportedVideoEncodeAcceleratorProfiles() {
  scoped_refptr<gpu::GpuChannelHost> gpu_channel_host =
      EstablishGpuChannelSync();
  if (!gpu_channel_host)
    return media::VideoEncodeAccelerator::SupportedProfiles();
  return media::GpuVideoAcceleratorUtil::ConvertGpuToMediaEncodeProfiles(
      gpu_channel_host->gpu_info().video_encode_accelerator_supported_profiles);
}

void MirrorService::CreateVideoEncodeAccelerator(
    const cast::ReceiveVideoEncodeAcceleratorCallback& callback) {
  DVLOG(1) << __func__;
  scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner =
      g_mirror_threads.Get().GetVideoEncodeTaskRunner();
  base::PostTaskAndReplyWithResult(
      encode_task_runner.get(), FROM_HERE,
      base::Bind(&MirrorService::CreateVea, base::Unretained(this)),
      base::Bind(callback, encode_task_runner));
}

std::unique_ptr<media::VideoEncodeAccelerator> MirrorService::CreateVea() {
  if (!vea_provider_.is_bound()) {
    gpu_->CreateVideoEncodeAcceleratorProvider(
        mojo::MakeRequest(&vea_provider_));
  }
  if (!vea_provider_)
    return nullptr;
  media::mojom::VideoEncodeAcceleratorPtr vea;
  vea_provider_->CreateVideoEncodeAccelerator(mojo::MakeRequest(&vea));
  if (!vea)
    return nullptr;
  scoped_refptr<gpu::GpuChannelHost> gpu_channel_host =
      EstablishGpuChannelSync();
  if (!gpu_channel_host)
    return nullptr;
  return std::unique_ptr<media::VideoEncodeAccelerator>(
      new media::MojoVideoEncodeAccelerator(
          std::move(vea), gpu_channel_host->gpu_info()
                              .video_encode_accelerator_supported_profiles));
}

void MirrorService::CreateVideoEncodeMemory(
    size_t size,
    const cast::ReceiveVideoEncodeMemoryCallback& callback) {
  DVLOG(1) << __func__;

  mojo::ScopedSharedBufferHandle mojo_buf =
      mojo::SharedBufferHandle::Create(size);
  if (!mojo_buf->is_valid()) {
    LOG(WARNING) << "Browser failed to allocate shared memory.";
    callback.Run(nullptr);
    return;
  }

  base::SharedMemoryHandle shared_buf;
  if (mojo::UnwrapSharedMemoryHandle(std::move(mojo_buf), &shared_buf, nullptr,
                                     nullptr) != MOJO_RESULT_OK) {
    LOG(WARNING) << "Browser failed to allocate shared memory.";
    callback.Run(nullptr);
    return;
  }

  callback.Run(base::MakeUnique<base::SharedMemory>(shared_buf, false));
}

scoped_refptr<gpu::GpuChannelHost> MirrorService::EstablishGpuChannelSync(
    bool* connection_error) {
  if (!gpu_) {
    base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
    const bool is_running_in_mash =
        cmdline->HasSwitch(switches::kIsRunningInMash);
    gpu_ = ui::Gpu::Create(content::UtilityThread::Get()->GetConnector(),
                           is_running_in_mash
                               ? ui::mojom::kServiceName
                               : content::mojom::kBrowserServiceName,
                           content::ChildProcess::current()->io_task_runner());
  }

  return gpu_->EstablishGpuChannelSync(connection_error);
}

}  // namespace media
