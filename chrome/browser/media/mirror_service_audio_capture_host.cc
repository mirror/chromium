// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/mirror_service_audio_capture_host.h"

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "base/sync_socket.h"
#include "content/browser/media/capture/audio_mirroring_manager.h"
#include "content/browser/media/capture/web_contents_audio_input_stream.h"
#include "content/browser/renderer_host/media/audio_input_sync_writer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "media/audio/audio_thread_impl.h"
#include "media/mojo/services/mojo_audio_input_stream.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using content::BrowserThread;

namespace media {

namespace {
constexpr int kAudioCaptureSharedMemoryCount = 10;
}  // namespace

AudioDelegate::AudioDelegate(
    AudioInputDelegate::EventHandler* event_handler,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner)
    : event_handler_(event_handler),
      audio_task_runner_(audio_task_runner),
      weak_factory_(this) {}

void AudioDelegate::CreateStream(const std::string& device_id,
                                 const AudioParameters& params) {
  std::unique_ptr<base::CancelableSyncSocket> foreign_socket =
      base::MakeUnique<base::CancelableSyncSocket>();
  writer_ = content::AudioInputSyncWriter::Create(
      kAudioCaptureSharedMemoryCount, params, foreign_socket.get());
  if (!writer_) {
    DVLOG(1) << "Failed to set up sync writer.";
    event_handler_->OnStreamError(0);
  }

  if (content::WebContentsMediaCaptureId::Parse(device_id, nullptr)) {
    DVLOG(1) << "Create WebContentsAudioInputStream frames_per_buffer = "
             << params.frames_per_buffer();
    stream_ = content::WebContentsAudioInputStream::Create(
        device_id, params, audio_task_runner_,
        content::AudioMirroringManager::GetInstance());
  } else {
    DVLOG(1) << "Wrong device id for audio capture.";
    event_handler_->OnStreamError(0);
  }

  if (!stream_) {
    DVLOG(1) << "Error to create audio capture stream.";
    event_handler_->OnStreamError(0);
    return;
  }

  if (!stream_->Open()) {
    DVLOG(1) << "Error to open the audio capture stream.";
    event_handler_->OnStreamError(0);
    return;
  }

  stream_->SetAutomaticGainControl(false);
  DVLOG(1) << "Audio stream is opened: is_muted= " << stream_->IsMuted();

  event_handler_->OnStreamCreated(0, writer_->shared_memory(),
                                  std::move(foreign_socket),
                                  stream_->IsMuted());
}

AudioDelegate::~AudioDelegate() {}

void AudioDelegate::Stop() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(1) << "Stop AudioInputStream.";
  if (stream_) {
    stream_->Stop();
    stream_->Close();
    stream_ = nullptr;
  }

  if (writer_)
    writer_->Close();
}

void AudioDelegate::OnRecordStream() {
  if (!stream_)
    event_handler_->OnStreamError(0);
  DVLOG(1) << "Start audio capture.";
  DCHECK(stream_);
  stream_->Start(this);
}

int AudioDelegate::GetStreamId() {
  return 0;
}

void AudioDelegate::OnSetVolume(double volume) {
  DCHECK_GE(volume, 0);
  DCHECK_LE(volume, 1);
  if (!stream_)
    return;

  DVLOG(1) << __func__ << " volume: " << volume;

  // Only ask for the maximum volume at first call and use cached value
  // for remaining function calls.
  if (!max_volume_) {
    max_volume_ = stream_->GetMaxVolume();
  }

  if (max_volume_ == 0.0) {
    DLOG(WARNING) << "Failed to access input volume control";
    return;
  }

  // Set the stream volume and scale to a range matched to the platform.
  stream_->SetVolume(max_volume_ * volume);
}

void AudioDelegate::OnData(const AudioBus* source,
                           base::TimeTicks capture_time,
                           double volume) {
  writer_->Write(source, volume, false /*key_pressed*/, capture_time);
}

void AudioDelegate::OnError() {
  DVLOG(1) << "AudioInputStream error.";
  event_handler_->OnStreamError(0);
}

MirrorServiceAudioCaptureHost::MirrorServiceAudioCaptureHost(
    const std::string& device_id,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner)
    : device_id_(device_id),
      audio_task_runner_(audio_task_runner),
      weak_factory_(this) {}

MirrorServiceAudioCaptureHost::~MirrorServiceAudioCaptureHost() {}

// static
void MirrorServiceAudioCaptureHost::Create(
    const std::string& device_id,
    content::MediaStreamType type,
    AudioCaptureHostCreatedCallback callback,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::unique_ptr<MirrorServiceAudioCaptureHost> host =
      base::MakeUnique<MirrorServiceAudioCaptureHost>(device_id,
                                                      audio_task_runner);
  host->GetAudioParameters(
      type, base::BindOnce(&MirrorServiceAudioCaptureHost::OnAudioParameters,
                           base::Passed(&host), base::Passed(&callback)));
}

void MirrorServiceAudioCaptureHost::OnAudioParameters(
    std::unique_ptr<MirrorServiceAudioCaptureHost> host,
    AudioCaptureHostCreatedCallback callback,
    const base::Optional<AudioParameters>& opt_params) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  AudioParameters params = AudioParameters::UnavailableDeviceParams();
  if (opt_params) {
    params = *opt_params;
    DVLOG(1) << "opt_params: " << opt_params->frames_per_buffer();
  }

  DCHECK(host);
  // TODO(xjz): HACK. Need to find out the root cause.
  params.set_frames_per_buffer(params.sample_rate() / 100);
  DVLOG(1) << "Before setting params: frames_per_buffer="
           << params.frames_per_buffer()
           << " sample_rate=" << params.sample_rate();

  host->SetParams(params);
  mojom::AudioCaptureHostPtr ptr;
  mojom::AudioCaptureHostRequest request = mojo::MakeRequest(&ptr);
  mojo::MakeStrongBinding(std::move(host), std::move(request));
  if (!callback.is_null())
    base::ResetAndReturn(&callback).Run(params, std::move(ptr));
}

// static
void MirrorServiceAudioCaptureHost::GetAudioParameters(
    content::MediaStreamType type,
    AudioSystem::OnAudioParamsCallback callback) {
  if (type == content::MEDIA_TAB_AUDIO_CAPTURE) {
    if (!audio_system_)
      audio_system_ = AudioSystem::CreateInstance();
    audio_system_->GetOutputStreamParameters(
        AudioDeviceDescription::kDefaultDeviceId, std::move(callback));
  } else {
    base::ResetAndReturn(&callback).Run(
        AudioParameters::UnavailableDeviceParams());
  }
}

void MirrorServiceAudioCaptureHost::SetParams(const AudioParameters& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  params_ = params;
}

void MirrorServiceAudioCaptureHost::CreateStream(
    mojom::AudioInputStreamClientPtr client,
    mojom::AudioInputStreamRequest stream,
    MojoAudioInputStream::StreamCreatedCallback cb) {
  DVLOG(1) << __func__;

  if (stream_) {
    DVLOG(1) << "Existing audio capture when request to start.";
    client->OnError();
    return;
  }

  stream_.reset(new MojoAudioInputStream(
      std::move(stream), std::move(client),
      base::BindOnce(&MirrorServiceAudioCaptureHost::GetAudioInputDelegate,
                     base::Unretained(this)),
      std::move(cb),
      base::BindOnce(&MirrorServiceAudioCaptureHost::Stop,
                     base::Unretained(this))));
  DCHECK(delegate_);
  DCHECK(params_.IsValid());
  delegate_->CreateStream(device_id_, params_);
}

std::unique_ptr<AudioInputDelegate>
MirrorServiceAudioCaptureHost::GetAudioInputDelegate(
    AudioInputDelegate::EventHandler* event_handler) {
  std::unique_ptr<AudioDelegate> delegate =
      base::MakeUnique<AudioDelegate>(event_handler, audio_task_runner_);
  DCHECK(!delegate_);
  delegate_ = delegate.get();
  return delegate;
}

void MirrorServiceAudioCaptureHost::Stop() {
  DVLOG(1) << __func__;
  if (delegate_) {
    delegate_->Stop();
  }
  stream_ = nullptr;
}

}  // namespace media
