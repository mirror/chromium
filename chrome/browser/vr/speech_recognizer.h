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

class SpeechRecognizerDelegate {
 public:
  // Receive a speech recognition result. |is_final| indicated whether the
  // result is an intermediate or final result. If |is_final| is true, then the
  // recognizer stops and no more results will be returned.
  virtual void OnSpeechResult(const base::string16& query, bool is_final) = 0;

  // Invoked regularly to indicate the average sound volume.
  virtual void OnSpeechSoundLevelChanged(int16_t level) = 0;

  // Invoked when the state of speech recognition is changed.
  // replace SpeechRecognitionState with int
  virtual void OnSpeechRecognitionStateChanged(int new_state) = 0;

  // Get the OAuth2 scope and token to pass to the speech recognizer. Does not
  // modify the arguments if no auth token is available or allowed.
  virtual void GetSpeechAuthParameters(std::string* auth_scope,
                                       std::string* auth_token) = 0;

 protected:
  virtual ~SpeechRecognizerDelegate() {}
};

// SpeechRecognizer is a wrapper around the speech recognition engine that
// simplifies its use from the UI thread. This class handles all setup/shutdown,
// collection of results, error cases, and threading.
class SpeechRecognizer {
 public:
  SpeechRecognizer(const base::WeakPtr<SpeechRecognizerDelegate>& delegate,
                   net::URLRequestContextGetter* url_request_context_getter,
                   const std::string& locale);
  ~SpeechRecognizer();

  // Start/stop the speech recognizer. |preamble| contains the preamble audio to
  // log if auth parameters are available.
  // Must be called on the UI thread.
  void Start(
      const scoped_refptr<content::SpeechRecognitionSessionPreamble>& preamble);
  void Stop();

 private:
  class EventListener;

  base::WeakPtr<SpeechRecognizerDelegate> delegate_;
  scoped_refptr<EventListener> speech_event_listener_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognizer);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_SPEECH_RECOGNIZER_H_
