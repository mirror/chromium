// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_INPUT_DELEGATE_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_INPUT_DELEGATE_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "media/audio/audio_input_delegate.h"

namespace base {
class FilePath;
}

namespace content {
class AudioInputSyncWriter;
class AudioMirroringManager;
}  // namespace content

namespace media {
class AudioLog;
class AudioManager;
class AudioInputController;
class AudioParameters;
}  // namespace media

namespace content {

// This class, except for the AudioInputDelegateImpl::EventHandler
// implementation, is operated on the IO thread.
class CONTENT_EXPORT AudioInputDelegateImpl : public media::AudioInputDelegate {
 public:
  class ControllerEventHandler;

  /*static std::unique_ptr<AudioInputDelegateImpl> Create(
      EventHandler* handler,
      media::AudioManager* audio_manager,
      std::unique_ptr<media::AudioLog> audio_log,
      AudioMirroringManager* mirroring_manager,
      int stream_id,
      int session_id,
      int render_frame_id,
      int render_process_id,
      const media::AudioParameters& params,
      const std::string& input_device_id);*/

  AudioInputDelegateImpl(
      std::unique_ptr<ControllerEventHandler> controller_event_handler,
      std::unique_ptr<AudioInputSyncWriter> writer,
      std::unique_ptr<base::CancelableSyncSocket> foreign_socket,
      std::unique_ptr<media::AudioLog> audio_log,
      scoped_refptr<media::AudioInputController> controller,
      EventHandler* handler,
      int stream_id,
      int render_frame_id,
      int render_process_id);

  ~AudioInputDelegateImpl() override;

  // AudioInputDelegate implementation.
  int GetStreamId() const override;
  void OnRecordStream() override;
  void OnSetVolume(double volume) override;
  void EnableDebugRecording(const base::FilePath& file_name) override;
  void DisableDebugRecording() override;

 private:
  void SendCreatedNotification(bool initially_muted);
  void OnError();

  // This is the event handler which |this| send notifications to.
  EventHandler* subscriber_;

  // |controller_event_handler_| proxies events from controller to |this|.
  // |controller_event_handler_|, |writer_| and |mirroring_manager_| will
  // outlive |this|, see the destructor for details.
  std::unique_ptr<ControllerEventHandler> controller_event_handler_;
  std::unique_ptr<AudioInputSyncWriter> writer_;
  std::unique_ptr<base::CancelableSyncSocket> foreign_socket_;
  std::unique_ptr<media::AudioLog> audio_log_;
  scoped_refptr<media::AudioInputController> controller_;
  const int stream_id_;
  const int render_frame_id_;
  const int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(AudioInputDelegateImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_AUDIO_INPUT_DELEGATE_IMPL_H_
