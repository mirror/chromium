// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SPEECH_SPEECH_RECOGNITION_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_SPEECH_SPEECH_RECOGNITION_DISPATCHER_HOST_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/common/speech_recognizer.mojom.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "ipc/ipc_sender.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

class SpeechRecognitionManager;

// SpeechRecognitionDispatcherHost is a delegate for Speech API messages used by
// RenderMessageFilter. Basically it acts as a proxy, relaying the events coming
// from the SpeechRecognitionManager to IPC messages (and vice versa).
// It's the complement of SpeechRecognitionDispatcher (owned by RenderView).
class CONTENT_EXPORT SpeechRecognitionDispatcherHost
    : public SpeechRecognitionEventListener,
      public mojom::SpeechRecognizer {
 public:
  SpeechRecognitionDispatcherHost(
      base::WeakPtr<IPC::Sender> sender,
      int render_process_id,
      scoped_refptr<net::URLRequestContextGetter> context_getter);

  ~SpeechRecognitionDispatcherHost() override;

  static void Create(base::WeakPtr<IPC::Sender> sender,
                     int render_process_id,
                     scoped_refptr<net::URLRequestContextGetter> context_getter,
                     mojom::SpeechRecognizerRequest request);

  base::WeakPtr<SpeechRecognitionDispatcherHost> AsWeakPtr();

  // SpeechRecognitionEventListener methods.
  void OnRecognitionStart(int session_id) override;
  void OnAudioStart(int session_id) override;
  void OnEnvironmentEstimationComplete(int session_id) override;
  void OnSoundStart(int session_id) override;
  void OnSoundEnd(int session_id) override;
  void OnAudioEnd(int session_id) override;
  void OnRecognitionEnd(int session_id) override;
  void OnRecognitionResults(int session_id,
                            const SpeechRecognitionResults& results) override;
  void OnRecognitionError(int session_id,
                          const SpeechRecognitionError& error) override;
  void OnAudioLevelsChange(int session_id,
                           float volume,
                           float noise_volume) override;

  // mojom::SpeechRecognizer:
  void StartRequest(
      mojom::StartSpeechRecognitionRequestParamsPtr params) override;
  void AbortRequest(int64_t render_view_id, int64_t request_id) override;
  void AbortAllRequests(int64_t render_view_id) override;
  void StopCaptureRequest(int64_t render_view_id, int64_t request_id) override;

 private:
  friend class base::DeleteHelper<SpeechRecognitionDispatcherHost>;
  friend class BrowserThread;

  static void StartRequestOnUI(
      base::WeakPtr<SpeechRecognitionDispatcherHost>
          speech_recognition_dispatcher_host,
      int render_process_id,
      mojom::StartSpeechRecognitionRequestParamsPtr params);

  void StartRequestOnIO(int embedder_render_process_id,
                        int embedder_render_view_id,
                        mojom::StartSpeechRecognitionRequestParamsPtr params,
                        int params_render_frame_id,
                        bool filter_profanities);

  // Sends the given message on the UI thread. Can be called from the IO thread.
  void SendOnUI(IPC::Message* message);

  // Checks the reported render view ID against the stored one. If they are
  // different, calls mojo::ReportBadMessage and returns true.
  bool CheckIfBadRenderViewId(int reported_render_view_id);

  int render_process_id_;
  int render_view_id_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;

  // TODO(sashab): Convert this message path to Mojo and remove it.
  base::WeakPtr<IPC::Sender> sender_;

  // Used for posting asynchronous tasks (on the IO thread) without worrying
  // about this class being destroyed in the meanwhile (due to browser shutdown)
  // since tasks pending on a destroyed WeakPtr are automatically discarded.
  base::WeakPtrFactory<SpeechRecognitionDispatcherHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognitionDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SPEECH_SPEECH_RECOGNITION_DISPATCHER_HOST_H_
