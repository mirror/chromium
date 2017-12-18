// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/speech/speech_recognition_dispatcher_host.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "content/browser/browser_plugin/browser_plugin_guest.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/speech/speech_recognition_manager_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/speech_recognition_messages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/speech_recognition_manager_delegate.h"
#include "content/public/browser/speech_recognition_session_config.h"
#include "content/public/browser/speech_recognition_session_context.h"
#include "content/public/common/content_switches.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

SpeechRecognitionDispatcherHost::SpeechRecognitionDispatcherHost(
    base::WeakPtr<IPC::Sender> sender,
    int render_process_id,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : render_process_id_(render_process_id),
      render_view_id_(MSG_ROUTING_NONE),
      context_getter_(std::move(context_getter)),
      sender_(sender),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Do not add any non-trivial initialization here, instead do it lazily when
  // required (e.g. see the method |SpeechRecognitionManager::GetInstance()|) or
  // add an Init() method.
}

SpeechRecognitionDispatcherHost::~SpeechRecognitionDispatcherHost() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // TODO(sashab): Maybe add DCHECK(!SpeechRecognitionManager::GetInstance())
  // TODO(sashab): Does this leak sessions?
  if (SpeechRecognitionManager::GetInstance())
    SpeechRecognitionManager::GetInstance()->AbortAllSessionsForRenderView(
        render_process_id_, render_view_id_);
}

// static
void SpeechRecognitionDispatcherHost::Create(
    base::WeakPtr<IPC::Sender> sender,
    int render_process_id,
    scoped_refptr<net::URLRequestContextGetter> context_getter,
    mojom::SpeechRecognizerRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<SpeechRecognitionDispatcherHost>(
          sender, render_process_id, std::move(context_getter)),
      std::move(request));
}

base::WeakPtr<SpeechRecognitionDispatcherHost>
SpeechRecognitionDispatcherHost::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void SpeechRecognitionDispatcherHost::StartRequest(
    mojom::StartSpeechRecognitionRequestParamsPtr params) {
  if (CheckIfBadRenderViewId(params->render_view_id)) {
    return;
  }

  render_view_id_ = params->render_view_id;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&SpeechRecognitionDispatcherHost::StartRequestOnUI,
                     AsWeakPtr(), render_process_id_, std::move(params)));
}

void SpeechRecognitionDispatcherHost::AbortRequest(int64_t render_view_id,
                                                   int64_t request_id) {
  if (CheckIfBadRenderViewId(render_view_id)) {
    return;
  }

  int session_id = SpeechRecognitionManager::GetInstance()->GetSession(
      render_process_id_, render_view_id, request_id);

  // The renderer might provide an invalid |request_id| if the session was not
  // started as expected, e.g., due to unsatisfied security requirements.
  if (session_id != SpeechRecognitionManager::kSessionIDInvalid)
    SpeechRecognitionManager::GetInstance()->AbortSession(session_id);
}

void SpeechRecognitionDispatcherHost::AbortAllRequests(int64_t render_view_id) {
  if (CheckIfBadRenderViewId(render_view_id)) {
    return;
  }

  SpeechRecognitionManager::GetInstance()->AbortAllSessionsForRenderView(
      render_process_id_, render_view_id);
}

void SpeechRecognitionDispatcherHost::StopCaptureRequest(int64_t render_view_id,
                                                         int64_t request_id) {
  if (CheckIfBadRenderViewId(render_view_id)) {
    return;
  }

  int session_id = SpeechRecognitionManager::GetInstance()->GetSession(
      render_process_id_, render_view_id, request_id);

  // The renderer might provide an invalid |request_id| if the session was not
  // started as expected, e.g., due to unsatisfied security requirements.
  if (session_id != SpeechRecognitionManager::kSessionIDInvalid) {
    SpeechRecognitionManager::GetInstance()->StopAudioCaptureForSession(
        session_id);
  }
}

// static
void SpeechRecognitionDispatcherHost::StartRequestOnUI(
    base::WeakPtr<SpeechRecognitionDispatcherHost>
        speech_recognition_dispatcher_host,
    int render_process_id,
    mojom::StartSpeechRecognitionRequestParamsPtr params) {
  // Check that the origin specified by the renderer process is one
  // that it is allowed to access.
  if (params->origin_url != "null" &&
      !ChildProcessSecurityPolicyImpl::GetInstance()->CanRequestURL(
          render_process_id, GURL(params->origin_url))) {
    LOG(ERROR) << "SRDH::OnStartRequest, disallowed origin: "
               << params->origin_url;
    return;
  }

  int embedder_render_process_id = 0;
  int embedder_render_view_id = MSG_ROUTING_NONE;
  RenderViewHostImpl* render_view_host =
      RenderViewHostImpl::FromID(render_process_id, params->render_view_id);
  if (!render_view_host) {
    // RVH can be null if the tab was closed while continuous mode speech
    // recognition was running. This seems to happen on mac.
    LOG(WARNING) << "SRDH::OnStartRequest, RenderViewHost does not exist";
    return;
  }

  WebContentsImpl* web_contents = static_cast<WebContentsImpl*>(
      WebContents::FromRenderViewHost(render_view_host));
  WebContentsImpl* outer_web_contents = web_contents->GetOuterWebContents();
  if (outer_web_contents) {
    // If the speech API request was from an inner WebContents or a guest, save
    // the context of the outer WebContents or the embedder since we will use it
    // to decide permission.
    embedder_render_process_id =
        outer_web_contents->GetRenderViewHost()->GetProcess()->GetID();
    DCHECK_NE(embedder_render_process_id, 0);
    embedder_render_view_id =
        outer_web_contents->GetRenderViewHost()->GetRoutingID();
    DCHECK_NE(embedder_render_view_id, MSG_ROUTING_NONE);
  }

  // TODO(lazyboy): Check if filter_profanities should use
  // |embedder_render_process_id| instead of |render_process_id|.
  bool filter_profanities =
      SpeechRecognitionManagerImpl::GetInstance() &&
      SpeechRecognitionManagerImpl::GetInstance()->delegate() &&
      SpeechRecognitionManagerImpl::GetInstance()
          ->delegate()
          ->FilterProfanities(render_process_id);

  // TODO(miu): This is a hack to allow SpeechRecognition to operate with the
  // MediaStreamManager, which partitions requests per RenderFrame, not per
  // RenderView.  http://crbug.com/390749
  const int params_render_frame_id = render_view_host ?
      render_view_host->GetMainFrame()->GetRoutingID() : MSG_ROUTING_NONE;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&SpeechRecognitionDispatcherHost::StartRequestOnIO,
                     speech_recognition_dispatcher_host,
                     embedder_render_process_id, embedder_render_view_id,
                     std::move(params), params_render_frame_id,
                     filter_profanities));
}

void SpeechRecognitionDispatcherHost::StartRequestOnIO(
    int embedder_render_process_id,
    int embedder_render_view_id,
    mojom::StartSpeechRecognitionRequestParamsPtr params,
    int params_render_frame_id,
    bool filter_profanities) {
  SpeechRecognitionSessionContext context;
  context.context_name = params->origin_url;
  context.render_process_id = render_process_id_;
  context.render_view_id = params->render_view_id;
  context.render_frame_id = params_render_frame_id;
  context.embedder_render_process_id = embedder_render_process_id;
  context.embedder_render_view_id = embedder_render_view_id;
  if (embedder_render_process_id)
    context.guest_render_view_id = params->render_view_id;
  context.request_id = params->request_id;

  SpeechRecognitionSessionConfig config;
  config.language = params->language;
  config.grammars = params->grammars;
  config.max_hypotheses = params->max_hypotheses;
  config.origin_url = params->origin_url;
  config.initial_context = context;
  config.url_request_context_getter = context_getter_.get();
  config.filter_profanities = filter_profanities;
  config.continuous = params->continuous;
  config.interim_results = params->interim_results;
  config.event_listener = AsWeakPtr();

  int session_id = SpeechRecognitionManager::GetInstance()->CreateSession(
      config);
  DCHECK_NE(session_id, SpeechRecognitionManager::kSessionIDInvalid);
  SpeechRecognitionManager::GetInstance()->StartSession(session_id);
}

void SpeechRecognitionDispatcherHost::SendOnUI(IPC::Message* message) {
  // The weak ptr must only be accessed on the UI thread.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(base::IgnoreResult(&IPC::Sender::Send), sender_, message));
}

bool SpeechRecognitionDispatcherHost::CheckIfBadRenderViewId(
    int reported_render_view_id) {
  if (render_view_id_ == MSG_ROUTING_NONE) {
    render_view_id_ = reported_render_view_id;
  } else if (render_view_id_ != reported_render_view_id) {
    mojo::ReportBadMessage(
        "SpeechRecognitionDispatcherHost called with bad render view ID.");
    return true;
  }

  return false;
}

// -------- SpeechRecognitionEventListener interface implementation -----------

void SpeechRecognitionDispatcherHost::OnRecognitionStart(int session_id) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_Started(context.render_view_id,
                                            context.request_id));
}

void SpeechRecognitionDispatcherHost::OnAudioStart(int session_id) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_AudioStarted(context.render_view_id,
                                                 context.request_id));
}

void SpeechRecognitionDispatcherHost::OnSoundStart(int session_id) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_SoundStarted(context.render_view_id,
                                                 context.request_id));
}

void SpeechRecognitionDispatcherHost::OnSoundEnd(int session_id) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_SoundEnded(context.render_view_id,
                                               context.request_id));
}

void SpeechRecognitionDispatcherHost::OnAudioEnd(int session_id) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_AudioEnded(context.render_view_id,
                                               context.request_id));
}

void SpeechRecognitionDispatcherHost::OnRecognitionEnd(int session_id) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_Ended(context.render_view_id,
                                          context.request_id));
}

void SpeechRecognitionDispatcherHost::OnRecognitionResults(
    int session_id,
    const SpeechRecognitionResults& results) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_ResultRetrieved(
      context.render_view_id, context.request_id, results));
}

void SpeechRecognitionDispatcherHost::OnRecognitionError(
    int session_id,
    const SpeechRecognitionError& error) {
  const SpeechRecognitionSessionContext& context =
      SpeechRecognitionManager::GetInstance()->GetSessionContext(session_id);
  DCHECK_EQ(render_view_id_, context.render_view_id);
  SendOnUI(new SpeechRecognitionMsg_ErrorOccurred(context.render_view_id,
                                                  context.request_id, error));
}

// The events below are currently not used by speech JS APIs implementation.
void SpeechRecognitionDispatcherHost::OnAudioLevelsChange(int session_id,
                                                          float volume,
                                                          float noise_volume) {
}

void SpeechRecognitionDispatcherHost::OnEnvironmentEstimationComplete(
    int session_id) {
}

}  // namespace content
