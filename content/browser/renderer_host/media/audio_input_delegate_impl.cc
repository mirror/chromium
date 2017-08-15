// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/audio_input_delegate_impl.h"

#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/strings/stringprintf.h"
#include "content/browser/renderer_host/media/audio_input_sync_writer.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/public/browser/browser_thread.h"
#include "media/audio/audio_input_controller.h"
#include "media/audio/audio_logging.h"

namespace content {

class AudioInputDelegateImpl::ControllerEventHandler
    : public media::AudioInputController::EventHandler {
 public:
  explicit ControllerEventHandler(int stream_id)
      : stream_id_(stream_id), delegate_(), weak_factory_(this) {}

  void OnCreated(media::AudioInputController* controller,
                 bool initially_muted) override {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ControllerEventHandler::OnCreatedOnIO,
                       weak_factory_.GetWeakPtr(), initially_muted));
  }

  void OnError(media::AudioInputController* controller,
               media::AudioInputController::ErrorCode error_code) override {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ControllerEventHandler::OnErrorOnIO,
                       weak_factory_.GetWeakPtr(), error_code));
  }

  void OnLog(media::AudioInputController* controller,
             const std::string& message) override {
    // Note: Log message has the AIRH prefix to ensure the log is compatible
    // with earlier Chrome versions.
    LogMessage(base::StringPrintf("[stream_id=%d] AIRH::%s", stream_id_,
                                  message.c_str()));
  }

  void set_delegate(AudioInputDelegateImpl* delegate) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    delegate_ = delegate;
  }

 private:
  void OnCreatedOnIO(bool initially_muted) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    if (delegate_)
      delegate_->SendCreatedNotification(initially_muted);
  }

  void OnErrorOnIO(media::AudioInputController::ErrorCode error_code) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    // To ensure that the error is logged even during the destruction sequence,
    // we log it here.
    LogMessage(base::StringPrintf("[stream_id=%d] AIC reports error_code=%d",
                                  stream_id_, error_code));
    if (delegate_)
      delegate_->OnError();
  }

  // Safe to call from any thread.
  static void LogMessage(const std::string& message) {
    MediaStreamManager::SendMessageToNativeLog(message);
    DVLOG(1) << message;
  }

  const int stream_id_;
  AudioInputDelegateImpl* delegate_;
  base::WeakPtrFactory<ControllerEventHandler> weak_factory_;
};

AudioInputDelegateImpl::AudioInputDelegateImpl(
    std::unique_ptr<ControllerEventHandler> controller_event_handler,
    std::unique_ptr<AudioInputSyncWriter> writer,
    std::unique_ptr<base::CancelableSyncSocket> foreign_socket,
    std::unique_ptr<media::AudioLog> audio_log,
    scoped_refptr<media::AudioInputController> controller,
    EventHandler* handler,
    int stream_id,
    int render_frame_id,
    int render_process_id)
    : subscriber_(handler),
      controller_event_handler_(std::move(controller_event_handler)),
      writer_(std::move(writer)),
      foreign_socket_(std::move(foreign_socket)),
      audio_log_(std::move(audio_log)),
      controller_(std::move(controller)),
      stream_id_(stream_id),
      render_frame_id_(render_frame_id),
      render_process_id_(render_process_id) {}

AudioInputDelegateImpl::~AudioInputDelegateImpl() {
  audio_log_->OnClosed(stream_id_);
  controller_event_handler_->set_delegate(nullptr);

  // We pass |controller_event_handler_| and |writer_| in here to make sure they
  // stay alive until |controller_| has finished closing.
  controller_->Close(
      base::BindOnce([](std::unique_ptr<ControllerEventHandler>,
                        std::unique_ptr<AudioInputSyncWriter>) {},
                     std::move(controller_event_handler_), std::move(writer_)));
}

// AudioInputDelegate implementation.
int AudioInputDelegateImpl::GetStreamId() const {
  return stream_id_;
}

void AudioInputDelegateImpl::OnRecordStream() {
  controller_->Record();
  audio_log_->OnStarted(stream_id_);
}

void AudioInputDelegateImpl::OnSetVolume(double volume) {
  DCHECK(volume < 0 || volume > 1);
  controller_->SetVolume(volume);
  audio_log_->OnSetVolume(stream_id_, volume);
}

void AudioInputDelegateImpl::EnableDebugRecording(
    const base::FilePath& file_name) {}

void AudioInputDelegateImpl::DisableDebugRecording() {}

void AudioInputDelegateImpl::SendCreatedNotification(bool initially_muted) {
  DCHECK(foreign_socket_);
  subscriber_->OnStreamCreated(stream_id_, writer_->shared_memory(),
                               std::move(foreign_socket_), initially_muted);
}

void AudioInputDelegateImpl::OnError() {
  subscriber_->OnStreamError(stream_id_);
}

}  // namespace content
