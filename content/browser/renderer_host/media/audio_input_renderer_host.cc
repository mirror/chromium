// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/audio_input_renderer_host.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/memory/shared_memory.h"
#include "base/metrics/histogram_macros.h"
#include "base/numerics/safe_math.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/sync_socket.h"
#include "build/build_config.h"
#include "content/browser/bad_message.h"
#include "content/browser/media/capture/desktop_capture_device_uma_types.h"
#include "content/browser/media/capture/web_contents_audio_input_stream.h"
#include "content/browser/media/media_internals.h"
#include "content/browser/renderer_host/media/audio_input_delegate_impl.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/browser/renderer_host/media/audio_input_sync_writer.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/browser/webrtc/webrtc_internals.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "media/audio/audio_device_description.h"
#include "media/base/audio_bus.h"
#include "media/base/media_switches.h"
#include "media/media_features.h"

namespace content {

namespace {

#if BUILDFLAG(ENABLE_WEBRTC)
const base::FilePath::CharType kDebugRecordingFileNameAddition[] =
    FILE_PATH_LITERAL("source_input");
#endif

void LogMessage(int stream_id, const std::string& msg, bool add_prefix) {
  std::ostringstream oss;
  oss << "[stream_id=" << stream_id << "] ";
  if (add_prefix)
    oss << "AIRH::";
  oss << msg;
  const std::string message = oss.str();
  content::MediaStreamManager::SendMessageToNativeLog(message);
  DVLOG(1) << message;
}

}  // namespace
AudioInputRendererHost::AudioInputRendererHost(
    int render_process_id,
    media::AudioManager* audio_manager,
    MediaStreamManager* media_stream_manager,
    AudioMirroringManager* audio_mirroring_manager,
    media::UserInputMonitor* user_input_monitor)
    : BrowserMessageFilter(AudioMsgStart),
      render_process_id_(render_process_id),
      renderer_pid_(0),
      audio_manager_(audio_manager),
      media_stream_manager_(media_stream_manager),
      audio_mirroring_manager_(audio_mirroring_manager),
      user_input_monitor_(user_input_monitor),
      audio_log_(MediaInternals::GetInstance()->CreateAudioLog(
          media::AudioLogFactory::AUDIO_INPUT_CONTROLLER)) {}

AudioInputRendererHost::~AudioInputRendererHost() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(delegates_.empty());
}

#if BUILDFLAG(ENABLE_WEBRTC)
void AudioInputRendererHost::EnableDebugRecording(const base::FilePath& file) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (delegates_.empty())
    return;
  base::FilePath file_with_extensions =
      GetDebugRecordingFilePathWithExtensions(file);
  for (const std::pair<int, std::unique_ptr<media::AudioInputDelegate>>& p :
       delegates_)
    EnableDebugRecordingForId(file_with_extensions, p.first);
}

void AudioInputRendererHost::DisableDebugRecording() {
  for (const std::pair<int, std::unique_ptr<media::AudioInputDelegate>>& p :
       delegates_)
    p.second->DisableDebugRecording();
}
#endif  // ENABLE_WEBRTC

void AudioInputRendererHost::OnChannelClosing() {
  // Since the IPC sender is gone, close all audio streams.
  delegates_.clear();
}

void AudioInputRendererHost::OnDestruct() const {
  BrowserThread::DeleteOnIOThread::Destruct(this);
}

void AudioInputRendererHost::OnStreamCreated(
    int stream_id,
    const base::SharedMemory* shared_memory,
    std::unique_ptr<base::CancelableSyncSocket> socket,
    bool initially_muted) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(PeerHandle());

  // Once the audio stream is created then complete the creation process by
  // mapping shared memory and sharing with the renderer process.
  base::SharedMemoryHandle foreign_memory_handle =
      shared_memory->handle().Duplicate();
  if (!foreign_memory_handle.IsValid()) {
    // If we failed to map and share the shared memory then close the audio
    // stream and send an error message.
    DeleteDelegateOnError(stream_id, MEMORY_SHARING_FAILED);
    return;
  }

  base::CancelableSyncSocket::TransitDescriptor socket_transit_descriptor;

  // If we failed to prepare the sync socket for the renderer then we fail
  // the construction of audio input stream.
  if (!socket->PrepareTransitDescriptor(PeerHandle(),
                                        &socket_transit_descriptor)) {
    foreign_memory_handle.Close();
    DeleteDelegateOnError(stream_id, SYNC_SOCKET_ERROR);
    return;
  }

  LogMessage(stream_id,
             base::StringPrintf("DoCompleteCreation: IPC channel and stream "
                                "are now open (initially %smuted)",
                                initially_muted ? "" : "not "),
             true);

  Send(new AudioInputMsg_NotifyStreamCreated(stream_id, foreign_memory_handle,
                                             socket_transit_descriptor,
                                             initially_muted));
}

void AudioInputRendererHost::OnStreamError(int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DeleteDelegateOnError(stream_id, AUDIO_INPUT_CONTROLLER_ERROR);
}

void AudioInputRendererHost::OnMuted(int stream_id, bool is_muted) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  Send(new AudioInputMsg_NotifyStreamMuted(stream_id, is_muted));
}

void AudioInputRendererHost::set_renderer_pid(int32_t renderer_pid) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  renderer_pid_ = renderer_pid;
}

bool AudioInputRendererHost::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AudioInputRendererHost, message)
    IPC_MESSAGE_HANDLER(AudioInputHostMsg_CreateStream, OnCreateStream)
    IPC_MESSAGE_HANDLER(AudioInputHostMsg_RecordStream, OnRecordStream)
    IPC_MESSAGE_HANDLER(AudioInputHostMsg_CloseStream, OnCloseStream)
    IPC_MESSAGE_HANDLER(AudioInputHostMsg_SetVolume, OnSetVolume)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AudioInputRendererHost::OnCreateStream(
    int stream_id,
    int render_frame_id,
    int session_id,
    const AudioInputHostMsg_CreateStream_Config& config) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

#if defined(OS_CHROMEOS)
  if (config.params.channel_layout() ==
      media::CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC) {
    media_stream_manager_->audio_input_device_manager()
        ->RegisterKeyboardMicStream(
            base::BindOnce(&AudioInputRendererHost::DoCreateStream, this,
                           stream_id, render_frame_id, session_id, config));
  } else {
    DoCreateStream(stream_id, render_frame_id, session_id, config,
                   AudioInputDeviceManager::KeyboardMicRegistration());
  }
#else
  DoCreateStream(stream_id, render_frame_id, session_id, config);
#endif
}

void AudioInputRendererHost::DoCreateStream(
    int stream_id,
    int render_frame_id,
    int session_id,
    const AudioInputHostMsg_CreateStream_Config& config
#if defined(OS_CHROMEOS)
    ,
    AudioInputDeviceManager::KeyboardMicRegistration keyboard_mic_registration
#endif
    ) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK_GT(render_frame_id, 0);

  // media::AudioParameters is validated in the deserializer.
  if (LookupById(stream_id)) {
    SendErrorMessage(stream_id, STREAM_ALREADY_EXISTS);
    return;
  }

  std::unique_ptr<media::AudioInputDelegate> delegate =
      AudioInputDelegateImpl::Create(
          this, audio_manager_, audio_mirroring_manager_, user_input_monitor_,
          media_stream_manager_,
          MediaInternals::GetInstance()->CreateAudioLog(
              media::AudioLogFactory::AUDIO_INPUT_CONTROLLER),
#if defined(OS_CHROMEOS)
          std::move(keyboard_mic_registration),
#endif
          config.shared_memory_count, stream_id, session_id, render_frame_id,
          render_process_id_, config.automatic_gain_control, config.params);

  if (!delegate) {
    // Error was logged by AudioInputDelegateImpl::Create.
    Send(new AudioInputMsg_NotifyStreamError(stream_id));

    return;
  }

  delegates_.emplace(stream_id, std::move(delegate));

#if BUILDFLAG(ENABLE_WEBRTC)
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AudioInputRendererHost::MaybeEnableDebugRecordingForId,
                     this, stream_id));
#endif
}

void AudioInputRendererHost::OnRecordStream(int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  LogMessage(stream_id, "OnRecordStream", true);

  media::AudioInputDelegate* delegate = LookupById(stream_id);
  if (!delegate) {
    SendErrorMessage(stream_id, INVALID_AUDIO_ENTRY);
    return;
  }

  delegate->OnRecordStream();
}

void AudioInputRendererHost::OnCloseStream(int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  LogMessage(stream_id, "OnCloseStream", true);

  delegates_.erase(stream_id);
}

void AudioInputRendererHost::OnSetVolume(int stream_id, double volume) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (volume < 0 || volume > 1) {
    bad_message::ReceivedBadMessage(this,
                                    bad_message::AIRH_VOLUME_OUT_OF_RANGE);
    return;
  }

  media::AudioInputDelegate* delegate = LookupById(stream_id);
  if (!delegate) {
    SendErrorMessage(stream_id, INVALID_AUDIO_ENTRY);
    return;
  }

  delegate->OnSetVolume(volume);
}

void AudioInputRendererHost::SendErrorMessage(
    int stream_id, ErrorCode error_code) {
  std::string err_msg =
      base::StringPrintf("SendErrorMessage(error_code=%d)", error_code);
  LogMessage(stream_id, err_msg, true);

  Send(new AudioInputMsg_NotifyStreamError(stream_id));
}

void AudioInputRendererHost::DeleteDelegateOnError(int stream_id,
                                                   ErrorCode error_code) {
  SendErrorMessage(stream_id, error_code);
  delegates_.erase(stream_id);
}

media::AudioInputDelegate* AudioInputRendererHost::LookupById(int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  AudioInputDelegateMap::iterator i = delegates_.find(stream_id);
  if (i != delegates_.end())
    return i->second.get();
  return nullptr;
}

#if BUILDFLAG(ENABLE_WEBRTC)
void AudioInputRendererHost::MaybeEnableDebugRecordingForId(int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (WebRTCInternals::GetInstance()->IsAudioDebugRecordingsEnabled()) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &AudioInputRendererHost::
                AddExtensionsToPathAndEnableDebugRecordingForId,
            this,
            WebRTCInternals::GetInstance()->GetAudioDebugRecordingsFilePath(),
            stream_id));
  }
}

#if defined(OS_WIN)
#define IntToStringType base::IntToString16
#else
#define IntToStringType base::IntToString
#endif

base::FilePath AudioInputRendererHost::GetDebugRecordingFilePathWithExtensions(
    const base::FilePath& file) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // We expect |renderer_pid_| to be set.
  DCHECK_GT(renderer_pid_, 0);
  return file.AddExtension(IntToStringType(renderer_pid_))
             .AddExtension(kDebugRecordingFileNameAddition);
}

void AudioInputRendererHost::EnableDebugRecordingForId(
    const base::FilePath& file_name,
    int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  media::AudioInputDelegate* delegate = LookupById(stream_id);
  if (!delegate)
    return;
  delegate->EnableDebugRecording(file_name);
}

#undef IntToStringType

void AudioInputRendererHost::AddExtensionsToPathAndEnableDebugRecordingForId(
    const base::FilePath& file,
    int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  EnableDebugRecordingForId(GetDebugRecordingFilePathWithExtensions(file),
                            stream_id);
}

#endif  // BUILDFLAG(ENABLE_WEBRTC)

}  // namespace content
