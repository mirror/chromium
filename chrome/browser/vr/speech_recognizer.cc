// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/speech_recognizer.h"

#include "base/bind.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/vr/browser_ui_interface.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "content/public/browser/speech_recognition_manager.h"
#include "content/public/browser/speech_recognition_session_config.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/speech_recognition_error.h"
#include "net/url_request/url_request_context_getter.h"

namespace vr {

namespace {

// Length of timeout to cancel recognition if there's no speech heard.
static const int kNoSpeechTimeoutInSeconds = 5;

// Length of timeout to cancel recognition if no different results are received.
static const int kNoNewSpeechTimeoutInSeconds = 2;

// Invalid speech session.
static const int kInvalidSessionId = -1;

// Not thread safe. Must only accessed on IO thread.
class SpeechRecognizerOnIO : public content::SpeechRecognitionEventListener {
 public:
  explicit SpeechRecognizerOnIO(
      const base::WeakPtr<IOBrowserUIInterface>& browser_ui);
  ~SpeechRecognizerOnIO() override;

  void Start(
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
      const std::string& locale,
      const std::string& auth_scope,
      const std::string& auth_token);

  // Starts a timer for |timeout_seconds|. When the timer expires, will stop
  // capturing audio and get a final utterance from the recognition manager.
  void StartSpeechTimeout(int timeout_seconds);
  void SpeechTimeout();

  // Overidden from content::SpeechRecognitionEventListener:
  // These are always called on the IO thread.
  void OnRecognitionStart(int session_id) override;
  void OnRecognitionEnd(int session_id) override;
  void OnRecognitionResults(
      int session_id,
      const content::SpeechRecognitionResults& results) override;
  void OnRecognitionError(
      int session_id,
      const content::SpeechRecognitionError& error) override;
  void OnSoundStart(int session_id) override;
  void OnSoundEnd(int session_id) override;
  void OnAudioLevelsChange(int session_id,
                           float volume,
                           float noise_volume) override;
  void OnEnvironmentEstimationComplete(int session_id) override;
  void OnAudioStart(int session_id) override;
  void OnAudioEnd(int session_id) override;

 private:
  void NotifyRecognitionStateChanged(SpeechRecognitionState new_state);

  // Only dereferenced from the UI thread, but copied on IO thread.
  base::WeakPtr<IOBrowserUIInterface> browser_ui_;

  // All remaining members only accessed from the IO thread.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  std::string locale_;
  base::Timer speech_timeout_;
  int session_;
  base::string16 last_result_str_;

  base::WeakPtrFactory<SpeechRecognizerOnIO> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognizerOnIO);
};

static SpeechRecognizerOnIO* g_speech_recognizer_on_io = nullptr;

void StartOnIOThread(
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
    base::WeakPtr<IOBrowserUIInterface> browser_ui,
    const std::string& locale,
    const std::string& auth_scope,
    const std::string& auth_token) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // Delete old session if any.
  delete g_speech_recognizer_on_io;

  g_speech_recognizer_on_io = new SpeechRecognizerOnIO(browser_ui);
  g_speech_recognizer_on_io->Start(url_request_context_getter, locale,
                                   auth_scope, auth_token);
}

void StopOnIOThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  delete g_speech_recognizer_on_io;
}

SpeechRecognizerOnIO::SpeechRecognizerOnIO(
    const base::WeakPtr<IOBrowserUIInterface>& browser_ui)
    : browser_ui_(browser_ui),
      speech_timeout_(false, false),
      session_(kInvalidSessionId),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(!g_speech_recognizer_on_io);
  g_speech_recognizer_on_io = this;
}

SpeechRecognizerOnIO::~SpeechRecognizerOnIO() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  content::SpeechRecognitionManager::GetInstance()->StopAudioCaptureForSession(
      session_);
  g_speech_recognizer_on_io = nullptr;
}

void SpeechRecognizerOnIO::Start(
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
    const std::string& locale,
    const std::string& auth_scope,
    const std::string& auth_token) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  content::SpeechRecognitionSessionConfig config;
  config.language = locale;
  config.continuous = true;
  config.interim_results = true;
  config.max_hypotheses = 1;
  config.filter_profanities = true;
  config.url_request_context_getter = url_request_context_getter;
  config.event_listener = weak_factory_.GetWeakPtr();
  // kInvalidUniqueID is not a valid render process, so the speech permission
  // check allows the request through.
  config.initial_context.render_process_id =
      content::ChildProcessHost::kInvalidUniqueID;
  config.auth_scope = auth_scope;
  config.auth_token = auth_token;

  auto* speech_instance = content::SpeechRecognitionManager::GetInstance();
  session_ = speech_instance->CreateSession(config);
  speech_instance->StartSession(session_);
}

void SpeechRecognizerOnIO::NotifyRecognitionStateChanged(
    SpeechRecognitionState new_state) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&IOBrowserUIInterface::OnSpeechRecognitionStateChanged,
                 browser_ui_, new_state));
}

void SpeechRecognizerOnIO::StartSpeechTimeout(int timeout_seconds) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  speech_timeout_.Start(FROM_HERE,
                        base::TimeDelta::FromSeconds(timeout_seconds),
                        base::Bind(&SpeechRecognizerOnIO::SpeechTimeout,
                                   weak_factory_.GetWeakPtr()));
}

void SpeechRecognizerOnIO::SpeechTimeout() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  NotifyRecognitionStateChanged(SPEECH_RECOGNITION_READY);
  StopOnIOThread();
}

void SpeechRecognizerOnIO::OnRecognitionStart(int session_id) {
  NotifyRecognitionStateChanged(SPEECH_RECOGNITION_RECOGNIZING);
}

void SpeechRecognizerOnIO::OnRecognitionEnd(int session_id) {
  NotifyRecognitionStateChanged(SPEECH_RECOGNITION_READY);
  StopOnIOThread();
}

void SpeechRecognizerOnIO::OnRecognitionResults(
    int session_id,
    const content::SpeechRecognitionResults& results) {
  base::string16 result_str;
  size_t final_count = 0;
  // The number of results with |is_provisional| false. If |final_count| ==
  // results.size(), then all results are non-provisional and the recognition is
  // complete.
  for (const auto& result : results) {
    if (!result.is_provisional)
      final_count++;
    result_str += result.hypotheses[0].utterance;
  }
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&IOBrowserUIInterface::OnSpeechResult, browser_ui_, result_str,
                 final_count == results.size()));

  // Stop the moment we have a final result. If we receive any new or changed
  // text, restart the timer to give the user more time to speak. (The timer is
  // recording the amount of time since the most recent utterance.)
  if (final_count == results.size())
    StopOnIOThread();
  else if (result_str != last_result_str_)
    StartSpeechTimeout(kNoNewSpeechTimeoutInSeconds);

  last_result_str_ = result_str;
}

void SpeechRecognizerOnIO::OnRecognitionError(
    int session_id,
    const content::SpeechRecognitionError& error) {
  if (error.code == content::SPEECH_RECOGNITION_ERROR_NETWORK) {
    NotifyRecognitionStateChanged(SPEECH_RECOGNITION_NETWORK_ERROR);
  }
}

void SpeechRecognizerOnIO::OnSoundStart(int session_id) {
  StartSpeechTimeout(kNoSpeechTimeoutInSeconds);
  NotifyRecognitionStateChanged(SPEECH_RECOGNITION_IN_SPEECH);
}

void SpeechRecognizerOnIO::OnSoundEnd(int session_id) {}

void SpeechRecognizerOnIO::OnAudioLevelsChange(int session_id,
                                               float volume,
                                               float noise_volume) {
  // Both |volume| and |noise_volume| are defined to be in the range [0.0, 1.0].
  DCHECK_LE(0.0, volume);
  DCHECK_GE(1.0, volume);
  DCHECK_LE(0.0, noise_volume);
  DCHECK_GE(1.0, noise_volume);
  volume = std::max(0.0f, volume - noise_volume);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&IOBrowserUIInterface::OnSpeechSoundLevelChanged, browser_ui_,
                 volume));
}

void SpeechRecognizerOnIO::OnEnvironmentEstimationComplete(int session_id) {}

void SpeechRecognizerOnIO::OnAudioStart(int session_id) {}

void SpeechRecognizerOnIO::OnAudioEnd(int session_id) {}

}  // namespace

SpeechRecognizer::SpeechRecognizer(
    BrowserUiInterface* ui,
    net::URLRequestContextGetter* url_request_context_getter,
    const std::string& locale)
    : ui_(ui),
      url_request_context_getter_(url_request_context_getter),
      locale_(locale),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

SpeechRecognizer::~SpeechRecognizer() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  Stop();
}

void SpeechRecognizer::Start() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string auth_scope;
  std::string auth_token;
  GetSpeechAuthParameters(&auth_scope, &auth_token);

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&StartOnIOThread, url_request_context_getter_,
                     weak_factory_.GetWeakPtr(), locale_, auth_scope,
                     auth_token));
}

void SpeechRecognizer::Stop() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  weak_factory_.InvalidateWeakPtrs();
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(&StopOnIOThread));
}

void SpeechRecognizer::OnSpeechResult(const base::string16& query,
                                      bool is_final) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!is_final)
    return;

  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(
          ProfileManager::GetActiveUserProfile());
  GURL url(GetDefaultSearchURLForSearchTerms(template_url_service, query));
  ui_->SetVoiceSearchResult(url.spec());
  // TODO(bshe): notify VR UI thread to draw UI.
}

void SpeechRecognizer::OnSpeechSoundLevelChanged(float level) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(bshe): notify VR UI thread to draw UI.
}

void SpeechRecognizer::OnSpeechRecognitionStateChanged(
    vr::SpeechRecognitionState new_state) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(bshe): notify VR UI thread state change.
}

void SpeechRecognizer::GetSpeechAuthParameters(std::string* auth_scope,
                                               std::string* auth_token) {
  // TODO(bshe): audio history requires auth_scope and auto_token. Get real
  // value if we need to save audio history.
}

}  // namespace vr
