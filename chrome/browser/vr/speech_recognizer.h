// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_SPEECH_RECOGNIZER_H_
#define CHROME_BROWSER_VR_SPEECH_RECOGNIZER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace content {
struct SpeechRecognitionSessionPreamble;
}

namespace net {
class URLRequestContextGetter;
}

namespace vr {

class BrowserUiInterface;

enum SpeechRecognitionState {
  SPEECH_RECOGNITION_OFF = 0,
  SPEECH_RECOGNITION_READY,
  SPEECH_RECOGNITION_HOTWORD_LISTENING,
  SPEECH_RECOGNITION_RECOGNIZING,
  SPEECH_RECOGNITION_IN_SPEECH,
  SPEECH_RECOGNITION_STOPPING,
  SPEECH_RECOGNITION_NETWORK_ERROR,
};

// An interface for IO to communicate with browser UI thread.
// This is used by SpeechRecognizerOnIO class who lives on IO thread.
class IOBrowserUIInterface {
 public:
  // Receive a speech recognition result. |is_final| indicated whether the
  // result is an intermediate or final result. If |is_final| is true, then the
  // recognizer stops and no more results will be returned.
  virtual void OnSpeechResult(const base::string16& query, bool is_final) = 0;

  // Invoked regularly to indicate the average sound volume.
  virtual void OnSpeechSoundLevelChanged(int16_t level) = 0;

  // Invoked when the state of speech recognition is changed.
  virtual void OnSpeechRecognitionStateChanged(
      SpeechRecognitionState new_state) = 0;

 protected:
  virtual ~IOBrowserUIInterface() {}
};

// SpeechRecognizer is a wrapper around the speech recognition engine that
// simplifies its use from the UI thread. This class handles all setup/shutdown,
// collection of results, error cases, and threading.
class SpeechRecognizer : public IOBrowserUIInterface {
 public:
  SpeechRecognizer(BrowserUiInterface* ui,
                   net::URLRequestContextGetter* url_request_context_getter,
                   const std::string& locale);
  ~SpeechRecognizer() override;

  // Start/stop the speech recognizer.
  // Must be called on the UI thread.
  void Start();
  void Stop();

  // Overridden from vr::IOBrowserUIInterface:
  void OnSpeechResult(const base::string16& query, bool is_final) override;
  void OnSpeechSoundLevelChanged(int16_t level) override;
  void OnSpeechRecognitionStateChanged(
      vr::SpeechRecognitionState new_state) override;

  void GetSpeechAuthParameters(std::string* auth_scope,
                               std::string* auth_token) {};

 private:
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  std::string locale_;

  BrowserUiInterface* ui_;
  base::WeakPtrFactory<SpeechRecognizer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognizer);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_SPEECH_RECOGNIZER_H_
