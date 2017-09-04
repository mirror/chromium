// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_INPUT_DELEGATE_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_INPUT_DELEGATE_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/common/content_export.h"
#include "media/audio/audio_input_delegate.h"

namespace base {
class FilePath;
}

namespace content {
class AudioInputSyncWriter;
class AudioMirroringManager;
class MediaStreamManager;
struct MediaStreamDevice;
}  // namespace content

namespace media {
class AudioLog;
class AudioManager;
class AudioInputController;
class AudioParameters;
class UserInputMonitor;
}  // namespace media

namespace content {

// This class, except for the AudioInputDelegateImpl::EventHandler
// implementation, is operated on the IO thread.
class CONTENT_EXPORT AudioInputDelegateImpl : public media::AudioInputDelegate {
 public:
  class ControllerEventHandler;

  AudioInputDelegateImpl(
      EventHandler* subscriber,
      media::AudioManager* audio_manager,
      AudioMirroringManager* mirroring_manager,
      media::UserInputMonitor* user_input_monitor,
      const media::AudioParameters& audio_parameters,
      std::unique_ptr<AudioInputSyncWriter> writer,
      std::unique_ptr<base::CancelableSyncSocket> foreign_socket,
      std::unique_ptr<media::AudioLog> audio_log,
#if defined(OS_CHROMEOS)
      AudioInputDeviceManager::KeyboardMicRegistration
          keyboard_mic_registration,
#endif
      int stream_id,
      int render_process_id,
      int render_frame_id,
      bool automatic_gain_control,
      const MediaStreamDevice* device);

  ~AudioInputDelegateImpl() override;

  static std::unique_ptr<media::AudioInputDelegate> Create(
      EventHandler* subscriber,
      media::AudioManager* audio_manager,
      AudioMirroringManager* mirroring_manager,
      media::UserInputMonitor* user_input_monitor,
      MediaStreamManager* media_stream_manager,
      std::unique_ptr<media::AudioLog> audio_log,
#if defined(OS_CHROMEOS)
      AudioInputDeviceManager::KeyboardMicRegistration
          keyboard_mic_registration,
#endif
      uint32_t shared_memory_count,
      int stream_id,
      int session_id,
      int render_frame_id,
      int render_process_id,
      bool automatic_gain_control,
      const media::AudioParameters& audio_parameters);

  // AudioInputDelegate implementation.
  int GetStreamId() const override;
  void OnRecordStream() override;
  void OnSetVolume(double volume) override;
  void EnableDebugRecording(const base::FilePath& file_name) override;
  void DisableDebugRecording() override;

 private:
  void SendCreatedNotification(bool initially_muted);
  void OnMuted(bool is_muted);
  void OnError();

  // This is the event handler which |this| send notifications to.
  EventHandler* const subscriber_;

  // |controller_event_handler_| proxies events from controller to |this|.
  // |controller_event_handler_| and |writer_| will
  // outlive |this|, see the destructor for details.
  std::unique_ptr<ControllerEventHandler> controller_event_handler_;
  std::unique_ptr<AudioInputSyncWriter> writer_;
  std::unique_ptr<base::CancelableSyncSocket> foreign_socket_;
  std::unique_ptr<media::AudioLog> audio_log_;
  scoped_refptr<media::AudioInputController> controller_;
#if defined(OS_CHROMEOS)
  const AudioInputDeviceManager::KeyboardMicRegistration
      keyboard_mic_registration_;
#endif
  const int stream_id_;
  const int render_process_id_;
  base::WeakPtrFactory<AudioInputDelegateImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioInputDelegateImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_INPUT_DELEGATE_IMPL_H_
