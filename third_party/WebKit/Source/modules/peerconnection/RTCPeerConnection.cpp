/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/peerconnection/RTCPeerConnection.h"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

#include "base/memory/ptr_util.h"
#include "bindings/core/v8/ExceptionMessages.h"
#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/v8_void_function.h"
#include "bindings/modules/v8/V8MediaStreamTrack.h"
#include "bindings/modules/v8/V8RTCCertificate.h"
#include "bindings/modules/v8/rtc_ice_candidate_init_or_rtc_ice_candidate.h"
#include "bindings/modules/v8/v8_rtc_peer_connection_error_callback.h"
#include "bindings/modules/v8/v8_rtc_session_description_callback.h"
#include "bindings/modules/v8/v8_rtc_stats_callback.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMTimeStamp.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/Deprecation.h"
#include "core/frame/HostsUsingFeatures.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameClient.h"
#include "core/frame/UseCounter.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "modules/crypto/CryptoResultImpl.h"
#include "modules/mediastream/MediaConstraintsImpl.h"
#include "modules/mediastream/MediaStream.h"
#include "modules/mediastream/MediaStreamEvent.h"
#include "modules/peerconnection/RTCAnswerOptions.h"
#include "modules/peerconnection/RTCConfiguration.h"
#include "modules/peerconnection/RTCDTMFSender.h"
#include "modules/peerconnection/RTCDataChannel.h"
#include "modules/peerconnection/RTCDataChannelEvent.h"
#include "modules/peerconnection/RTCDataChannelInit.h"
#include "modules/peerconnection/RTCIceServer.h"
#include "modules/peerconnection/RTCOfferOptions.h"
#include "modules/peerconnection/RTCPeerConnectionIceEvent.h"
#include "modules/peerconnection/RTCRtpReceiver.h"
#include "modules/peerconnection/RTCRtpSender.h"
#include "modules/peerconnection/RTCSessionDescription.h"
#include "modules/peerconnection/RTCSessionDescriptionInit.h"
#include "modules/peerconnection/RTCSessionDescriptionRequestImpl.h"
#include "modules/peerconnection/RTCSessionDescriptionRequestPromiseImpl.h"
#include "modules/peerconnection/RTCStatsReport.h"
#include "modules/peerconnection/RTCStatsRequestImpl.h"
#include "modules/peerconnection/RTCTrackEvent.h"
#include "modules/peerconnection/RTCVoidRequestImpl.h"
#include "modules/peerconnection/RTCVoidRequestPromiseImpl.h"
#include "modules/peerconnection/testing/InternalsRTCPeerConnection.h"
#include "platform/InstanceCounters.h"
#include "platform/bindings/Microtask.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/V8ThrowException.h"
#include "platform/peerconnection/RTCAnswerOptionsPlatform.h"
#include "platform/peerconnection/RTCOfferOptionsPlatform.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/Optional.h"
#include "platform/wtf/Time.h"
#include "public/platform/Platform.h"
#include "public/platform/TaskType.h"
#include "public/platform/WebCryptoAlgorithmParams.h"
#include "public/platform/WebMediaStream.h"
#include "public/platform/WebRTCAnswerOptions.h"
#include "public/platform/WebRTCCertificate.h"
#include "public/platform/WebRTCCertificateGenerator.h"
#include "public/platform/WebRTCConfiguration.h"
#include "public/platform/WebRTCDataChannelHandler.h"
#include "public/platform/WebRTCDataChannelInit.h"
#include "public/platform/WebRTCError.h"
#include "public/platform/WebRTCICECandidate.h"
#include "public/platform/WebRTCKeyParams.h"
#include "public/platform/WebRTCOfferOptions.h"
#include "public/platform/WebRTCSessionDescription.h"
#include "public/platform/WebRTCSessionDescriptionRequest.h"
#include "public/platform/WebRTCStatsRequest.h"
#include "public/platform/WebRTCVoidRequest.h"

namespace blink {

namespace {

const char kSignalingStateClosedMessage[] =
    "The RTCPeerConnection's signalingState is 'closed'.";

// The maximum number of PeerConnections that can exist simultaneously.
const long kMaxPeerConnections = 500;

bool ThrowExceptionIfSignalingStateClosed(
    RTCPeerConnection::SignalingState state,
    ExceptionState& exception_state) {
  if (state == RTCPeerConnection::kSignalingStateClosed) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      kSignalingStateClosedMessage);
    return true;
  }

  return false;
}

void AsyncCallErrorCallback(V8RTCPeerConnectionErrorCallback* error_callback,
                            DOMException* exception) {
  DCHECK(error_callback);
  Microtask::EnqueueMicrotask(
      WTF::Bind(&V8RTCPeerConnectionErrorCallback::InvokeAndReportException,
                WrapPersistentCallbackFunction(error_callback), nullptr,
                WrapPersistent(exception)));
}

bool CallErrorCallbackIfSignalingStateClosed(
    RTCPeerConnection::SignalingState state,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  if (state == RTCPeerConnection::kSignalingStateClosed) {
    if (error_callback)
      AsyncCallErrorCallback(
          error_callback, DOMException::Create(kInvalidStateError,
                                               kSignalingStateClosedMessage));

    return true;
  }

  return false;
}

bool IsIceCandidateMissingSdp(
    const RTCIceCandidateInitOrRTCIceCandidate& candidate) {
  if (candidate.IsRTCIceCandidateInit()) {
    const RTCIceCandidateInit& ice_candidate_init =
        candidate.GetAsRTCIceCandidateInit();
    return !ice_candidate_init.hasSdpMid() &&
           !ice_candidate_init.hasSdpMLineIndex();
  }

  DCHECK(candidate.IsRTCIceCandidate());
  return false;
}

WebRTCOfferOptions ConvertToWebRTCOfferOptions(const RTCOfferOptions& options) {
  return WebRTCOfferOptions(RTCOfferOptionsPlatform::Create(
      options.hasOfferToReceiveVideo()
          ? std::max(options.offerToReceiveVideo(), 0)
          : -1,
      options.hasOfferToReceiveAudio()
          ? std::max(options.offerToReceiveAudio(), 0)
          : -1,
      options.hasVoiceActivityDetection() ? options.voiceActivityDetection()
                                          : true,
      options.hasIceRestart() ? options.iceRestart() : false));
}

WebRTCAnswerOptions ConvertToWebRTCAnswerOptions(
    const RTCAnswerOptions& options) {
  return WebRTCAnswerOptions(RTCAnswerOptionsPlatform::Create(
      options.hasVoiceActivityDetection() ? options.voiceActivityDetection()
                                          : true));
}

scoped_refptr<WebRTCICECandidate> ConvertToWebRTCIceCandidate(
    ExecutionContext* context,
    const RTCIceCandidateInitOrRTCIceCandidate& candidate) {
  DCHECK(!candidate.IsNull());
  if (candidate.IsRTCIceCandidateInit()) {
    const RTCIceCandidateInit& ice_candidate_init =
        candidate.GetAsRTCIceCandidateInit();
    // TODO(guidou): Change default value to -1. crbug.com/614958.
    unsigned short sdp_m_line_index = 0;
    if (ice_candidate_init.hasSdpMLineIndex()) {
      sdp_m_line_index = ice_candidate_init.sdpMLineIndex();
    } else {
      UseCounter::Count(context,
                        WebFeature::kRTCIceCandidateDefaultSdpMLineIndex);
    }
    return WebRTCICECandidate::Create(ice_candidate_init.candidate(),
                                      ice_candidate_init.sdpMid(),
                                      sdp_m_line_index);
  }

  DCHECK(candidate.IsRTCIceCandidate());
  return candidate.GetAsRTCIceCandidate()->WebCandidate();
}

// Helper class for RTCPeerConnection::generateCertificate.
class WebRTCCertificateObserver : public WebRTCCertificateCallback {
 public:
  // Takes ownership of |resolver|.
  static WebRTCCertificateObserver* Create(ScriptPromiseResolver* resolver) {
    return new WebRTCCertificateObserver(resolver);
  }

  ~WebRTCCertificateObserver() override = default;

 private:
  explicit WebRTCCertificateObserver(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}

  void OnSuccess(std::unique_ptr<WebRTCCertificate> certificate) override {
    resolver_->Resolve(new RTCCertificate(std::move(certificate)));
  }

  void OnError() override { resolver_->Reject(); }

  Persistent<ScriptPromiseResolver> resolver_;
};

WebRTCIceTransportPolicy IceTransportPolicyFromString(const String& policy) {
  if (policy == "relay")
    return WebRTCIceTransportPolicy::kRelay;
  DCHECK_EQ(policy, "all");
  return WebRTCIceTransportPolicy::kAll;
}

WebRTCConfiguration ParseConfiguration(ExecutionContext* context,
                                       const RTCConfiguration& configuration,
                                       ExceptionState& exception_state) {
  DCHECK(context);

  WebRTCIceTransportPolicy ice_transport_policy =
      WebRTCIceTransportPolicy::kAll;
  if (configuration.hasIceTransportPolicy()) {
    UseCounter::Count(context, WebFeature::kRTCConfigurationIceTransportPolicy);
    ice_transport_policy =
        IceTransportPolicyFromString(configuration.iceTransportPolicy());
  } else if (configuration.hasIceTransports()) {
    UseCounter::Count(context, WebFeature::kRTCConfigurationIceTransports);
    ice_transport_policy =
        IceTransportPolicyFromString(configuration.iceTransports());
  }

  WebRTCBundlePolicy bundle_policy = WebRTCBundlePolicy::kBalanced;
  String bundle_policy_string = configuration.bundlePolicy();
  if (bundle_policy_string == "max-compat") {
    bundle_policy = WebRTCBundlePolicy::kMaxCompat;
  } else if (bundle_policy_string == "max-bundle") {
    bundle_policy = WebRTCBundlePolicy::kMaxBundle;
  } else {
    DCHECK_EQ(bundle_policy_string, "balanced");
  }

  WebRTCRtcpMuxPolicy rtcp_mux_policy = WebRTCRtcpMuxPolicy::kRequire;
  String rtcp_mux_policy_string = configuration.rtcpMuxPolicy();
  if (rtcp_mux_policy_string == "negotiate") {
    rtcp_mux_policy = WebRTCRtcpMuxPolicy::kNegotiate;
    Deprecation::CountDeprecation(context, WebFeature::kRtcpMuxPolicyNegotiate);
  } else {
    DCHECK_EQ(rtcp_mux_policy_string, "require");
  }

  WebRTCSdpSemantics sdp_semantics = WebRTCSdpSemantics::kDefault;
  if (configuration.hasSdpSemantics()) {
    String sdp_semantics_string = configuration.sdpSemantics();
    if (sdp_semantics_string == "plan-b") {
      sdp_semantics = WebRTCSdpSemantics::kPlanB;
    } else {
      DCHECK_EQ(sdp_semantics_string, "unified-plan");
      sdp_semantics = WebRTCSdpSemantics::kUnifiedPlan;
    }
  }

  WebRTCConfiguration web_configuration;
  web_configuration.ice_transport_policy = ice_transport_policy;
  web_configuration.bundle_policy = bundle_policy;
  web_configuration.rtcp_mux_policy = rtcp_mux_policy;
  web_configuration.sdp_semantics = sdp_semantics;

  if (configuration.hasIceServers()) {
    Vector<WebRTCIceServer> ice_servers;
    for (const RTCIceServer& ice_server : configuration.iceServers()) {
      Vector<String> url_strings;
      if (ice_server.hasURLs()) {
        UseCounter::Count(context, WebFeature::kRTCIceServerURLs);
        const StringOrStringSequence& urls = ice_server.urls();
        if (urls.IsString()) {
          url_strings.push_back(urls.GetAsString());
        } else {
          DCHECK(urls.IsStringSequence());
          url_strings = urls.GetAsStringSequence();
        }
      } else if (ice_server.hasURL()) {
        UseCounter::Count(context, WebFeature::kRTCIceServerURL);
        url_strings.push_back(ice_server.url());
      } else {
        exception_state.ThrowTypeError("Malformed RTCIceServer");
        return WebRTCConfiguration();
      }

      String username = ice_server.username();
      String credential = ice_server.credential();

      for (const String& url_string : url_strings) {
        KURL url(NullURL(), url_string);
        if (!url.IsValid()) {
          exception_state.ThrowDOMException(
              kSyntaxError, "'" + url_string + "' is not a valid URL.");
          return WebRTCConfiguration();
        }
        if (!(url.ProtocolIs("turn") || url.ProtocolIs("turns") ||
              url.ProtocolIs("stun"))) {
          exception_state.ThrowDOMException(
              kSyntaxError, "'" + url.Protocol() +
                                "' is not one of the supported URL schemes "
                                "'stun', 'turn' or 'turns'.");
          return WebRTCConfiguration();
        }
        if ((url.ProtocolIs("turn") || url.ProtocolIs("turns")) &&
            (username.IsNull() || credential.IsNull())) {
          exception_state.ThrowDOMException(kInvalidAccessError,
                                            "Both username and credential are "
                                            "required when the URL scheme is "
                                            "\"turn\" or \"turns\".");
        }
        ice_servers.push_back(WebRTCIceServer{url, username, credential});
      }
    }
    web_configuration.ice_servers = ice_servers;
  }

  if (configuration.hasCertificates()) {
    const HeapVector<Member<RTCCertificate>>& certificates =
        configuration.certificates();
    WebVector<std::unique_ptr<WebRTCCertificate>> certificates_copy(
        certificates.size());
    for (size_t i = 0; i < certificates.size(); ++i) {
      certificates_copy[i] = certificates[i]->CertificateShallowCopy();
    }
    web_configuration.certificates = std::move(certificates_copy);
  }

  web_configuration.ice_candidate_pool_size =
      configuration.iceCandidatePoolSize();
  return web_configuration;
}

RTCOfferOptionsPlatform* ParseOfferOptions(const Dictionary& options,
                                           ExceptionState& exception_state) {
  if (options.IsUndefinedOrNull())
    return nullptr;

  const Vector<String>& property_names =
      options.GetPropertyNames(exception_state);
  if (exception_state.HadException())
    return nullptr;

  // Treat |options| as MediaConstraints if it is empty or has "optional" or
  // "mandatory" properties for compatibility.
  // TODO(jiayl): remove constraints when RTCOfferOptions reaches Stable and
  // client code is ready.
  if (property_names.IsEmpty() || property_names.Contains("optional") ||
      property_names.Contains("mandatory"))
    return nullptr;

  int32_t offer_to_receive_video = -1;
  int32_t offer_to_receive_audio = -1;
  bool voice_activity_detection = true;
  bool ice_restart = false;

  if (DictionaryHelper::Get(options, "offerToReceiveVideo",
                            offer_to_receive_video) &&
      offer_to_receive_video < 0)
    offer_to_receive_video = 0;
  if (DictionaryHelper::Get(options, "offerToReceiveAudio",
                            offer_to_receive_audio) &&
      offer_to_receive_audio < 0)
    offer_to_receive_audio = 0;
  DictionaryHelper::Get(options, "voiceActivityDetection",
                        voice_activity_detection);
  DictionaryHelper::Get(options, "iceRestart", ice_restart);

  RTCOfferOptionsPlatform* rtc_offer_options = RTCOfferOptionsPlatform::Create(
      offer_to_receive_video, offer_to_receive_audio, voice_activity_detection,
      ice_restart);
  return rtc_offer_options;
}

// Helper class for
// |RTCPeerConnection::getStats(ScriptState*, MediaStreamTrack*)|
class WebRTCStatsReportCallbackResolver : public WebRTCStatsReportCallback {
 public:
  // Takes ownership of |resolver|.
  static std::unique_ptr<WebRTCStatsReportCallback> Create(
      ScriptPromiseResolver* resolver) {
    return std::unique_ptr<WebRTCStatsReportCallback>(
        new WebRTCStatsReportCallbackResolver(resolver));
  }

  ~WebRTCStatsReportCallbackResolver() override {
    DCHECK(
        ExecutionContext::From(resolver_->GetScriptState())->IsContextThread());
  }

 private:
  explicit WebRTCStatsReportCallbackResolver(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}

  void OnStatsDelivered(std::unique_ptr<WebRTCStatsReport> report) override {
    DCHECK(
        ExecutionContext::From(resolver_->GetScriptState())->IsContextThread());
    resolver_->Resolve(new RTCStatsReport(std::move(report)));
  }

  Persistent<ScriptPromiseResolver> resolver_;
};

}  // namespace

RTCPeerConnection::EventWrapper::EventWrapper(Event* event,
                                              BoolFunction function)
    : event_(event), setup_function_(std::move(function)) {}

bool RTCPeerConnection::EventWrapper::Setup() {
  if (setup_function_) {
    return std::move(setup_function_).Run();
  }
  return true;
}

void RTCPeerConnection::EventWrapper::Trace(blink::Visitor* visitor) {
  visitor->Trace(event_);
}

RTCPeerConnection* RTCPeerConnection::Create(
    ExecutionContext* context,
    const RTCConfiguration& rtc_configuration,
    const Dictionary& media_constraints,
    ExceptionState& exception_state) {
  // Count number of PeerConnections that could potentially be impacted by CSP
  if (context) {
    auto& security_context = context->GetSecurityContext();
    auto content_security_policy = security_context.GetContentSecurityPolicy();
    if (content_security_policy &&
        content_security_policy->IsActiveForConnections()) {
      UseCounter::Count(context, WebFeature::kRTCPeerConnectionWithActiveCsp);
    }
  }

  if (media_constraints.IsObject()) {
    UseCounter::Count(context,
                      WebFeature::kRTCPeerConnectionConstructorConstraints);
  } else {
    UseCounter::Count(context,
                      WebFeature::kRTCPeerConnectionConstructorCompliant);
  }

  WebRTCConfiguration configuration =
      ParseConfiguration(context, rtc_configuration, exception_state);
  if (exception_state.HadException())
    return nullptr;

  // Make sure no certificates have expired.
  if (configuration.certificates.size() > 0) {
    DOMTimeStamp now = ConvertSecondsToDOMTimeStamp(CurrentTime());
    for (const std::unique_ptr<WebRTCCertificate>& certificate :
         configuration.certificates) {
      DOMTimeStamp expires = certificate->Expires();
      if (expires <= now) {
        exception_state.ThrowDOMException(kInvalidAccessError,
                                          "Expired certificate(s).");
        return nullptr;
      }
    }
  }

  MediaErrorState media_error_state;
  WebMediaConstraints constraints = MediaConstraintsImpl::Create(
      context, media_constraints, media_error_state);
  if (media_error_state.HadException()) {
    media_error_state.RaiseException(exception_state);
    return nullptr;
  }

  RTCPeerConnection* peer_connection = new RTCPeerConnection(
      context, configuration, constraints, exception_state);
  peer_connection->PauseIfNeeded();
  if (exception_state.HadException())
    return nullptr;

  return peer_connection;
}

RTCPeerConnection::RTCPeerConnection(ExecutionContext* context,
                                     const WebRTCConfiguration& configuration,
                                     WebMediaConstraints constraints,
                                     ExceptionState& exception_state)
    : PausableObject(context),
      signaling_state_(kSignalingStateStable),
      ice_gathering_state_(kICEGatheringStateNew),
      ice_connection_state_(kICEConnectionStateNew),
      // WebRTC spec specifies kNetworking as task source.
      // https://www.w3.org/TR/webrtc/#operation
      dispatch_scheduled_event_runner_(
          AsyncMethodRunner<RTCPeerConnection>::Create(
              this,
              &RTCPeerConnection::DispatchScheduledEvent,
              context->GetTaskRunner(TaskType::kNetworking))),
      stopped_(false),
      closed_(false),
      has_data_channels_(false) {
  Document* document = ToDocument(GetExecutionContext());

  // If we fail, set |m_closed| and |m_stopped| to true, to avoid hitting the
  // assert in the destructor.

  if (InstanceCounters::CounterValue(
          InstanceCounters::kRTCPeerConnectionCounter) >= kMaxPeerConnections) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(kUnknownError,
                                      "Cannot create so many PeerConnections");
    return;
  }
  InstanceCounters::IncrementCounter(
      InstanceCounters::kRTCPeerConnectionCounter);
  if (!document->GetFrame()) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "PeerConnections may not be created in detached documents.");
    return;
  }

  peer_handler_ = Platform::Current()->CreateRTCPeerConnectionHandler(
      this, document->GetTaskRunner(TaskType::kUnthrottled));
  if (!peer_handler_) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "No PeerConnection handler can be "
                                      "created, perhaps WebRTC is disabled?");
    return;
  }

  document->GetFrame()->Client()->DispatchWillStartUsingPeerConnectionHandler(
      peer_handler_.get());

  if (!peer_handler_->Initialize(configuration, constraints)) {
    closed_ = true;
    stopped_ = true;
    exception_state.ThrowDOMException(
        kNotSupportedError, "Failed to initialize native PeerConnection.");
    return;
  }

  connection_handle_for_scheduler_ =
      document->GetFrame()->FrameScheduler()->OnActiveConnectionCreated();
}

RTCPeerConnection::~RTCPeerConnection() {
  // This checks that close() or stop() is called before the destructor.
  // We are assuming that a wrapper is always created when RTCPeerConnection is
  // created.
  DCHECK(closed_ || stopped_);
  InstanceCounters::DecrementCounter(
      InstanceCounters::kRTCPeerConnectionCounter);
}

void RTCPeerConnection::Dispose() {
  // Promptly clears a raw reference from content/ to an on-heap object
  // so that content/ doesn't access it in a lazy sweeping phase.
  peer_handler_.reset();
}

ScriptPromise RTCPeerConnection::createOffer(ScriptState* script_state,
                                             const RTCOfferOptions& options) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestPromiseImpl::Create(this, resolver);
  if (options.hasOfferToReceiveAudio() || options.hasOfferToReceiveVideo()) {
    ExecutionContext* context = ExecutionContext::From(script_state);
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionCreateOfferOptionsOfferToReceive);
  }
  peer_handler_->CreateOffer(request, ConvertToWebRTCOfferOptions(options));
  return promise;
}

ScriptPromise RTCPeerConnection::createOffer(
    ScriptState* script_state,
    V8RTCSessionDescriptionCallback* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback,
    const Dictionary& rtc_offer_options,
    ExceptionState& exception_state) {
  DCHECK(success_callback);
  DCHECK(error_callback);
  ExecutionContext* context = ExecutionContext::From(script_state);
  UseCounter::Count(
      context, WebFeature::kRTCPeerConnectionCreateOfferLegacyFailureCallback);
  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  RTCOfferOptionsPlatform* offer_options =
      ParseOfferOptions(rtc_offer_options, exception_state);
  if (exception_state.HadException())
    return ScriptPromise();
  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestImpl::Create(
          GetExecutionContext(), this, success_callback, error_callback);

  if (offer_options) {
    if (offer_options->OfferToReceiveAudio() != -1 ||
        offer_options->OfferToReceiveVideo() != -1) {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyOfferOptions);
    } else {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyCompliant);
    }

    peer_handler_->CreateOffer(request, WebRTCOfferOptions(offer_options));
  } else {
    MediaErrorState media_error_state;
    WebMediaConstraints constraints = MediaConstraintsImpl::Create(
        context, rtc_offer_options, media_error_state);
    // Report constraints parsing errors via the callback, but ignore
    // unknown/unsupported constraints as they would be silently discarded by
    // WebIDL.
    if (media_error_state.CanGenerateException()) {
      String error_msg = media_error_state.GetErrorMessage();
      AsyncCallErrorCallback(error_callback,
                             DOMException::Create(kOperationError, error_msg));
      return ScriptPromise::CastUndefined(script_state);
    }

    if (!constraints.IsEmpty()) {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyConstraints);
    } else {
      UseCounter::Count(
          context, WebFeature::kRTCPeerConnectionCreateOfferLegacyCompliant);
    }

    peer_handler_->CreateOffer(request, constraints);
  }

  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise RTCPeerConnection::createAnswer(ScriptState* script_state,
                                              const RTCAnswerOptions& options) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestPromiseImpl::Create(this, resolver);
  peer_handler_->CreateAnswer(request, ConvertToWebRTCAnswerOptions(options));
  return promise;
}

ScriptPromise RTCPeerConnection::createAnswer(
    ScriptState* script_state,
    V8RTCSessionDescriptionCallback* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback,
    const Dictionary& media_constraints) {
  DCHECK(success_callback);
  DCHECK(error_callback);
  ExecutionContext* context = ExecutionContext::From(script_state);
  UseCounter::Count(
      context, WebFeature::kRTCPeerConnectionCreateAnswerLegacyFailureCallback);
  if (media_constraints.IsObject()) {
    UseCounter::Count(
        context, WebFeature::kRTCPeerConnectionCreateAnswerLegacyConstraints);
  } else {
    UseCounter::Count(
        context, WebFeature::kRTCPeerConnectionCreateAnswerLegacyCompliant);
  }

  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  MediaErrorState media_error_state;
  WebMediaConstraints constraints = MediaConstraintsImpl::Create(
      context, media_constraints, media_error_state);
  // Report constraints parsing errors via the callback, but ignore
  // unknown/unsupported constraints as they would be silently discarded by
  // WebIDL.
  if (media_error_state.CanGenerateException()) {
    String error_msg = media_error_state.GetErrorMessage();
    AsyncCallErrorCallback(error_callback,
                           DOMException::Create(kOperationError, error_msg));
    return ScriptPromise::CastUndefined(script_state);
  }

  RTCSessionDescriptionRequest* request =
      RTCSessionDescriptionRequestImpl::Create(
          GetExecutionContext(), this, success_callback, error_callback);
  peer_handler_->CreateAnswer(request, constraints);
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise RTCPeerConnection::setLocalDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCVoidRequest* request = RTCVoidRequestPromiseImpl::Create(this, resolver);
  peer_handler_->SetLocalDescription(
      request, WebRTCSessionDescription(session_description_init.type(),
                                        session_description_init.sdp()));
  return promise;
}

ScriptPromise RTCPeerConnection::setLocalDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (success_callback && error_callback) {
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionSetLocalDescriptionLegacyCompliant);
  } else {
    if (!success_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetLocalDescriptionLegacyNoSuccessCallback);
    if (!error_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetLocalDescriptionLegacyNoFailureCallback);
  }

  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  RTCVoidRequest* request = RTCVoidRequestImpl::Create(
      GetExecutionContext(), this, success_callback, error_callback);
  peer_handler_->SetLocalDescription(
      request, WebRTCSessionDescription(session_description_init.type(),
                                        session_description_init.sdp()));
  return ScriptPromise::CastUndefined(script_state);
}

RTCSessionDescription* RTCPeerConnection::localDescription() {
  WebRTCSessionDescription web_session_description =
      peer_handler_->LocalDescription();
  if (web_session_description.IsNull())
    return nullptr;

  return RTCSessionDescription::Create(web_session_description);
}

ScriptPromise RTCPeerConnection::setRemoteDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCVoidRequest* request = RTCVoidRequestPromiseImpl::Create(this, resolver);
  peer_handler_->SetRemoteDescription(
      request, WebRTCSessionDescription(session_description_init.type(),
                                        session_description_init.sdp()));
  return promise;
}

ScriptPromise RTCPeerConnection::setRemoteDescription(
    ScriptState* script_state,
    const RTCSessionDescriptionInit& session_description_init,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (success_callback && error_callback) {
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionSetRemoteDescriptionLegacyCompliant);
  } else {
    if (!success_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetRemoteDescriptionLegacyNoSuccessCallback);
    if (!error_callback)
      UseCounter::Count(
          context,
          WebFeature::
              kRTCPeerConnectionSetRemoteDescriptionLegacyNoFailureCallback);
  }

  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  RTCVoidRequest* request = RTCVoidRequestImpl::Create(
      GetExecutionContext(), this, success_callback, error_callback);
  peer_handler_->SetRemoteDescription(
      request, WebRTCSessionDescription(session_description_init.type(),
                                        session_description_init.sdp()));
  return ScriptPromise::CastUndefined(script_state);
}

RTCSessionDescription* RTCPeerConnection::remoteDescription() {
  WebRTCSessionDescription web_session_description =
      peer_handler_->RemoteDescription();
  if (web_session_description.IsNull())
    return nullptr;

  return RTCSessionDescription::Create(web_session_description);
}

void RTCPeerConnection::setConfiguration(
    ScriptState* script_state,
    const RTCConfiguration& rtc_configuration,
    ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;

  WebRTCConfiguration configuration = ParseConfiguration(
      ExecutionContext::From(script_state), rtc_configuration, exception_state);

  if (exception_state.HadException())
    return;

  MediaErrorState media_error_state;
  if (media_error_state.HadException()) {
    media_error_state.RaiseException(exception_state);
    return;
  }

  WebRTCErrorType error = peer_handler_->SetConfiguration(configuration);
  if (error != WebRTCErrorType::kNone) {
    // All errors besides InvalidModification should have been detected above.
    if (error == WebRTCErrorType::kInvalidModification) {
      exception_state.ThrowDOMException(
          kInvalidModificationError,
          "Attempted to modify the PeerConnection's "
          "configuration in an unsupported way.");
    } else {
      exception_state.ThrowDOMException(
          kOperationError,
          "Could not update the PeerConnection with the given configuration.");
    }
  }
}

ScriptPromise RTCPeerConnection::generateCertificate(
    ScriptState* script_state,
    const AlgorithmIdentifier& keygen_algorithm,
    ExceptionState& exception_state) {
  // Normalize |keygenAlgorithm| with WebCrypto, making sure it is a recognized
  // AlgorithmIdentifier.
  WebCryptoAlgorithm crypto_algorithm;
  AlgorithmError error;
  if (!NormalizeAlgorithm(keygen_algorithm, kWebCryptoOperationGenerateKey,
                          crypto_algorithm, &error)) {
    // Reject generateCertificate with the same error as was produced by
    // WebCrypto. |result| is garbage collected, no need to delete.
    CryptoResultImpl* result = CryptoResultImpl::Create(script_state);
    ScriptPromise promise = result->Promise();
    result->CompleteWithError(error.error_type, error.error_details);
    return promise;
  }

  // Check if |keygenAlgorithm| contains the optional DOMTimeStamp |expires|
  // attribute.
  Optional<DOMTimeStamp> expires;
  if (keygen_algorithm.IsDictionary()) {
    Dictionary keygen_algorithm_dict = keygen_algorithm.GetAsDictionary();
    if (keygen_algorithm_dict.HasProperty("expires", exception_state)) {
      v8::Local<v8::Value> expires_value;
      keygen_algorithm_dict.Get("expires", expires_value);
      if (expires_value->IsNumber()) {
        double expires_double =
            expires_value
                ->ToNumber(script_state->GetIsolate()->GetCurrentContext())
                .ToLocalChecked()
                ->Value();
        if (expires_double >= 0) {
          expires = static_cast<DOMTimeStamp>(expires_double);
        }
      }
    }
  }
  if (exception_state.HadException()) {
    return ScriptPromise();
  }

  // Convert from WebCrypto representation to recognized WebRTCKeyParams. WebRTC
  // supports a small subset of what are valid AlgorithmIdentifiers.
  const char* unsupported_params_string =
      "The 1st argument provided is an AlgorithmIdentifier with a supported "
      "algorithm name, but the parameters are not supported.";
  Optional<WebRTCKeyParams> key_params;
  switch (crypto_algorithm.Id()) {
    case kWebCryptoAlgorithmIdRsaSsaPkcs1v1_5:
      // name: "RSASSA-PKCS1-v1_5"
      unsigned public_exponent;
      // "publicExponent" must fit in an unsigned int. The only recognized
      // "hash" is "SHA-256".
      if (crypto_algorithm.RsaHashedKeyGenParams()
              ->ConvertPublicExponentToUnsigned(public_exponent) &&
          crypto_algorithm.RsaHashedKeyGenParams()->GetHash().Id() ==
              kWebCryptoAlgorithmIdSha256) {
        unsigned modulus_length =
            crypto_algorithm.RsaHashedKeyGenParams()->ModulusLengthBits();
        key_params =
            WebRTCKeyParams::CreateRSA(modulus_length, public_exponent);
      } else {
        return ScriptPromise::RejectWithDOMException(
            script_state, DOMException::Create(kNotSupportedError,
                                               unsupported_params_string));
      }
      break;
    case kWebCryptoAlgorithmIdEcdsa:
      // name: "ECDSA"
      // The only recognized "namedCurve" is "P-256".
      if (crypto_algorithm.EcKeyGenParams()->NamedCurve() ==
          kWebCryptoNamedCurveP256) {
        key_params = WebRTCKeyParams::CreateECDSA(kWebRTCECCurveNistP256);
      } else {
        return ScriptPromise::RejectWithDOMException(
            script_state, DOMException::Create(kNotSupportedError,
                                               unsupported_params_string));
      }
      break;
    default:
      return ScriptPromise::RejectWithDOMException(
          script_state, DOMException::Create(kNotSupportedError,
                                             "The 1st argument provided is an "
                                             "AlgorithmIdentifier, but the "
                                             "algorithm is not supported."));
      break;
  }
  DCHECK(key_params.has_value());

  std::unique_ptr<WebRTCCertificateGenerator> certificate_generator =
      Platform::Current()->CreateRTCCertificateGenerator();

  // |keyParams| was successfully constructed, but does the certificate
  // generator support these parameters?
  if (!certificate_generator->IsSupportedKeyParams(key_params.value())) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kNotSupportedError, unsupported_params_string));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  std::unique_ptr<WebRTCCertificateObserver> certificate_observer(
      WebRTCCertificateObserver::Create(resolver));

  // Generate certificate. The |certificateObserver| will resolve the promise
  // asynchronously upon completion. The observer will manage its own
  // destruction as well as the resolver's destruction.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      ExecutionContext::From(script_state)
          ->GetTaskRunner(blink::TaskType::kUnthrottled);
  if (!expires) {
    certificate_generator->GenerateCertificate(
        key_params.value(), std::move(certificate_observer), task_runner);
  } else {
    certificate_generator->GenerateCertificateWithExpiration(
        key_params.value(), expires.value(), std::move(certificate_observer),
        task_runner);
  }

  return promise;
}

ScriptPromise RTCPeerConnection::addIceCandidate(
    ScriptState* script_state,
    const RTCIceCandidateInitOrRTCIceCandidate& candidate) {
  if (signaling_state_ == kSignalingStateClosed)
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kSignalingStateClosedMessage));

  if (IsIceCandidateMissingSdp(candidate))
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(
            script_state->GetIsolate(),
            "Candidate missing values for both sdpMid and sdpMLineIndex"));

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  RTCVoidRequest* request = RTCVoidRequestPromiseImpl::Create(this, resolver);
  scoped_refptr<WebRTCICECandidate> web_candidate = ConvertToWebRTCIceCandidate(
      ExecutionContext::From(script_state), candidate);
  bool implemented =
      peer_handler_->AddICECandidate(request, std::move(web_candidate));
  if (!implemented)
    resolver->Reject(DOMException::Create(
        kOperationError, "This operation could not be completed."));

  return promise;
}

ScriptPromise RTCPeerConnection::addIceCandidate(
    ScriptState* script_state,
    const RTCIceCandidateInitOrRTCIceCandidate& candidate,
    V8VoidFunction* success_callback,
    V8RTCPeerConnectionErrorCallback* error_callback) {
  DCHECK(success_callback);
  DCHECK(error_callback);

  if (CallErrorCallbackIfSignalingStateClosed(signaling_state_, error_callback))
    return ScriptPromise::CastUndefined(script_state);

  if (IsIceCandidateMissingSdp(candidate))
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(
            script_state->GetIsolate(),
            "Candidate missing values for both sdpMid and sdpMLineIndex"));

  RTCVoidRequest* request = RTCVoidRequestImpl::Create(
      GetExecutionContext(), this, success_callback, error_callback);
  scoped_refptr<WebRTCICECandidate> web_candidate = ConvertToWebRTCIceCandidate(
      ExecutionContext::From(script_state), candidate);
  bool implemented =
      peer_handler_->AddICECandidate(request, std::move(web_candidate));
  if (!implemented)
    AsyncCallErrorCallback(
        error_callback,
        DOMException::Create(kOperationError,
                             "This operation could not be completed."));

  return ScriptPromise::CastUndefined(script_state);
}

String RTCPeerConnection::signalingState() const {
  switch (signaling_state_) {
    case kSignalingStateStable:
      return "stable";
    case kSignalingStateHaveLocalOffer:
      return "have-local-offer";
    case kSignalingStateHaveRemoteOffer:
      return "have-remote-offer";
    case kSignalingStateHaveLocalPrAnswer:
      return "have-local-pranswer";
    case kSignalingStateHaveRemotePrAnswer:
      return "have-remote-pranswer";
    case kSignalingStateClosed:
      return "closed";
  }

  NOTREACHED();
  return String();
}

String RTCPeerConnection::iceGatheringState() const {
  switch (ice_gathering_state_) {
    case kICEGatheringStateNew:
      return "new";
    case kICEGatheringStateGathering:
      return "gathering";
    case kICEGatheringStateComplete:
      return "complete";
  }

  NOTREACHED();
  return String();
}

String RTCPeerConnection::iceConnectionState() const {
  switch (ice_connection_state_) {
    case kICEConnectionStateNew:
      return "new";
    case kICEConnectionStateChecking:
      return "checking";
    case kICEConnectionStateConnected:
      return "connected";
    case kICEConnectionStateCompleted:
      return "completed";
    case kICEConnectionStateFailed:
      return "failed";
    case kICEConnectionStateDisconnected:
      return "disconnected";
    case kICEConnectionStateClosed:
      return "closed";
  }

  NOTREACHED();
  return String();
}

void RTCPeerConnection::addStream(ScriptState* script_state,
                                  MediaStream* stream,
                                  const Dictionary& media_constraints,
                                  ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;

  if (!stream) {
    exception_state.ThrowDOMException(
        kTypeMismatchError,
        ExceptionMessages::ArgumentNullOrIncorrectType(1, "MediaStream"));
    return;
  }

  if (getLocalStreams().Contains(stream))
    return;

  MediaErrorState media_error_state;
  WebMediaConstraints constraints =
      MediaConstraintsImpl::Create(ExecutionContext::From(script_state),
                                   media_constraints, media_error_state);
  if (media_error_state.HadException()) {
    media_error_state.RaiseException(exception_state);
    return;
  }

  stream->RegisterObserver(this);
  for (auto& track : stream->getTracks()) {
    DCHECK(track->Component());
    tracks_.insert(track->Component(), track);
  }

  bool valid = peer_handler_->AddStream(stream->Descriptor(), constraints);
  if (!valid) {
    exception_state.ThrowDOMException(kSyntaxError,
                                      "Unable to add the provided stream.");
    return;
  }
  CreateMissingSendersForStream(stream);
}

void RTCPeerConnection::removeStream(MediaStream* stream,
                                     ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;

  if (!stream) {
    exception_state.ThrowDOMException(
        kTypeMismatchError,
        ExceptionMessages::ArgumentNullOrIncorrectType(1, "MediaStream"));
    return;
  }

  if (!getLocalStreams().Contains(stream))
    return;

  stream->UnregisterObserver(this);

  peer_handler_->RemoveStream(stream->Descriptor());
  RemoveUnusedSenders();
}

MediaStreamVector RTCPeerConnection::getLocalStreams() const {
  MediaStreamVector local_streams;
  for (const auto& rtp_sender : getSenders()) {
    for (const auto& stream : rtp_sender->streams()) {
      if (!local_streams.Contains(stream))
        local_streams.push_back(stream);
    }
  }
  return local_streams;
}

MediaStreamVector RTCPeerConnection::getRemoteStreams() const {
  MediaStreamVector remote_streams;
  for (const auto& rtp_receiver : rtp_receivers_) {
    for (const auto& stream : rtp_receiver->streams()) {
      if (!remote_streams.Contains(stream))
        remote_streams.push_back(stream);
    }
  }
  return remote_streams;
}

MediaStream* RTCPeerConnection::getRemoteStream(
    MediaStreamDescriptor* descriptor) const {
  for (const auto& rtp_receiver : rtp_receivers_) {
    for (const auto& stream : rtp_receiver->streams()) {
      if (stream->Descriptor() == descriptor)
        return stream;
    }
  }
  return nullptr;
}

size_t RTCPeerConnection::getRemoteStreamUsageCount(
    MediaStreamDescriptor* descriptor) const {
  size_t usage_count = 0;
  for (const auto& receiver : rtp_receivers_) {
    WebVector<WebMediaStream> streams = receiver->web_receiver().Streams();
    for (const WebMediaStream& stream : streams) {
      if (stream == descriptor)
        ++usage_count;
    }
  }
  return usage_count;
}

ScriptPromise RTCPeerConnection::getStats(ScriptState* script_state,
                                          V8RTCStatsCallback* success_callback,
                                          MediaStreamTrack* selector) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  UseCounter::Count(context,
                    WebFeature::kRTCPeerConnectionGetStatsLegacyNonCompliant);
  RTCStatsRequest* stats_request = RTCStatsRequestImpl::Create(
      GetExecutionContext(), this, success_callback, selector);
  // FIXME: Add passing selector as part of the statsRequest.
  peer_handler_->GetStats(stats_request);

  resolver->Resolve();
  return promise;
}

ScriptPromise RTCPeerConnection::getStats(ScriptState* script_state) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  UseCounter::Count(context, WebFeature::kRTCPeerConnectionGetStats);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  peer_handler_->GetStats(WebRTCStatsReportCallbackResolver::Create(resolver));

  return promise;
}

HeapVector<Member<RTCRtpSender>> RTCPeerConnection::getSenders() const {
  HeapVector<Member<RTCRtpSender>> rtp_senders;
  // |rtp_senders_| and lower layer GetSenders() should match, but we still
  // invoke GetSenders() to get a predictable list order.
  // TODO(hbos): Change |rtp_senders_| from a map to a list and stop relying on
  // GetSenders(). https://crbug.com/803021
  WebVector<std::unique_ptr<WebRTCRtpSender>> web_rtp_senders =
      peer_handler_->GetSenders();
  for (size_t i = 0; i < web_rtp_senders.size(); ++i) {
    uintptr_t id = web_rtp_senders[i]->Id();
    const auto it = rtp_senders_.find(id);
    DCHECK(it != rtp_senders_.end());
    rtp_senders.push_back(it->value);
  }
  return rtp_senders;
}

HeapVector<Member<RTCRtpReceiver>> RTCPeerConnection::getReceivers() const {
  return rtp_receivers_;
}

RTCRtpSender* RTCPeerConnection::addTrack(MediaStreamTrack* track,
                                          MediaStreamVector streams,
                                          ExceptionState& exception_state) {
  DCHECK(track);
  DCHECK(track->Component());
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return nullptr;
  if (streams.size() >= 2) {
    // TODO(hbos): Don't throw an exception when this is supported by the lower
    // layers. https://crbug.com/webrtc/7932
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "Adding a track to multiple streams is not supported.");
    return nullptr;
  }
  for (const auto sender_entry : rtp_senders_) {
    RTCRtpSender* sender = sender_entry.value;
    if (sender->track() == track) {
      exception_state.ThrowDOMException(
          kInvalidAccessError, "A sender already exists for the track.");
      return nullptr;
    }
  }

  WebVector<WebMediaStream> web_streams(streams.size());
  for (size_t i = 0; i < streams.size(); ++i) {
    web_streams[i] = streams[i]->Descriptor();
  }
  std::unique_ptr<WebRTCRtpSender> web_rtp_sender =
      peer_handler_->AddTrack(track->Component(), web_streams);
  if (!web_rtp_sender) {
    exception_state.ThrowDOMException(
        kNotSupportedError, "A sender could not be created for this track.");
    return nullptr;
  }

  uintptr_t id = web_rtp_sender->Id();
  DCHECK(rtp_senders_.find(id) == rtp_senders_.end());
  RTCRtpSender* rtp_sender =
      new RTCRtpSender(this, std::move(web_rtp_sender), track, streams);
  tracks_.insert(track->Component(), track);
  rtp_senders_.insert(id, rtp_sender);
  return rtp_sender;
}

void RTCPeerConnection::removeTrack(RTCRtpSender* sender,
                                    ExceptionState& exception_state) {
  DCHECK(sender);
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return;
  if (rtp_senders_.find(sender->web_sender()->Id()) == rtp_senders_.end()) {
    exception_state.ThrowDOMException(
        kInvalidAccessError,
        "The sender was not created by this peer connection.");
    return;
  }

  if (!peer_handler_->RemoveTrack(sender->web_sender())) {
    // Operation aborted. This indicates that the sender is no longer used by
    // the peer connection, i.e. that it was removed due to setting a remote
    // description of type "rollback".
    // Note: Until the WebRTC library supports re-using senders, a sender will
    // also stop being used as a result of being removed.
    return;
  }
  // Successfully removing the track results in the sender's track property
  // being nulled.
  DCHECK(!sender->web_sender()->Track());
  sender->SetTrack(nullptr);
  rtp_senders_.erase(sender->web_sender()->Id());
}

RTCDataChannel* RTCPeerConnection::createDataChannel(
    ScriptState* script_state,
    String label,
    const RTCDataChannelInit& data_channel_dict,
    ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return nullptr;

  WebRTCDataChannelInit init;
  init.ordered = data_channel_dict.ordered();
  ExecutionContext* context = ExecutionContext::From(script_state);
  if (data_channel_dict.hasMaxRetransmitTime()) {
    UseCounter::Count(
        context,
        WebFeature::kRTCPeerConnectionCreateDataChannelMaxRetransmitTime);
    init.max_retransmit_time = data_channel_dict.maxRetransmitTime();
  }
  if (data_channel_dict.hasMaxRetransmits()) {
    UseCounter::Count(
        context, WebFeature::kRTCPeerConnectionCreateDataChannelMaxRetransmits);
    init.max_retransmits = data_channel_dict.maxRetransmits();
  }
  init.protocol = data_channel_dict.protocol();
  init.negotiated = data_channel_dict.negotiated();
  if (data_channel_dict.hasId())
    init.id = data_channel_dict.id();

  RTCDataChannel* channel = RTCDataChannel::Create(
      GetExecutionContext(), peer_handler_.get(), label, init, exception_state);
  if (exception_state.HadException())
    return nullptr;
  RTCDataChannel::ReadyState handler_state = channel->GetHandlerState();
  if (handler_state != RTCDataChannel::kReadyStateConnecting) {
    // There was an early state transition.  Don't miss it!
    channel->DidChangeReadyState(handler_state);
  }
  has_data_channels_ = true;

  return channel;
}

MediaStreamTrack* RTCPeerConnection::GetTrack(
    const WebMediaStreamTrack& web_track) const {
  return tracks_.at(static_cast<MediaStreamComponent*>(web_track));
}

HeapVector<Member<RTCRtpReceiver>>::iterator RTCPeerConnection::FindReceiver(
    const WebRTCRtpReceiver& web_receiver) {
  for (auto it = rtp_receivers_.begin(); it != rtp_receivers_.end(); ++it) {
    if ((*it)->web_receiver().Id() == web_receiver.Id())
      return it;
  }
  return rtp_receivers_.end();
}

void RTCPeerConnection::CreateMissingSendersForStream(MediaStream* stream) {
  WebVector<std::unique_ptr<WebRTCRtpSender>> web_rtp_senders =
      peer_handler_->GetSenders();
  for (size_t i = 0; i < web_rtp_senders.size(); ++i) {
    uintptr_t id = web_rtp_senders[i]->Id();
    const auto it = rtp_senders_.find(id);
    // Any senders created by other means such as addTrack() will already exist
    // in |rtp_senders_| and not be examined further.
    if (it != rtp_senders_.end())
      continue;
    // Any sender that exist in the lower layer but not in blink must have been
    // created as a result of the operation that called
    // CreateMissingSendersForStream(). We create a blink sender under the
    // assumption that it is associated with |stream|.
    DCHECK(web_rtp_senders[i]->Track());
    MediaStreamTrack* track = GetTrack(web_rtp_senders[i]->Track());
    DCHECK(track);
    DCHECK(stream->getTracks().Contains(track));
    HeapVector<Member<MediaStream>> streams;
    streams.push_back(stream);
    RTCRtpSender* rtp_sender =
        new RTCRtpSender(this, std::move(web_rtp_senders[i]), track, streams);
    rtp_senders_.insert(id, rtp_sender);
  }
}

void RTCPeerConnection::RemoveUnusedSenders() {
  std::set<uintptr_t> used_sender_ids;
  for (const auto& web_rtp_sender : peer_handler_->GetSenders()) {
    used_sender_ids.insert(web_rtp_sender->Id());
  }
  // HeapHashMap does not support remove-while-iterating so we save all the IDs
  // of senders we want to erase.
  std::set<uintptr_t> unused_sender_ids;
  for (const auto& rtp_sender : rtp_senders_.Values()) {
    uintptr_t id = rtp_sender->web_sender()->Id();
    if (used_sender_ids.find(id) == used_sender_ids.end())
      unused_sender_ids.insert(id);
  }
  for (auto sender_id : unused_sender_ids) {
    rtp_senders_.erase(sender_id);
  }
}

RTCDTMFSender* RTCPeerConnection::createDTMFSender(
    MediaStreamTrack* track,
    ExceptionState& exception_state) {
  if (ThrowExceptionIfSignalingStateClosed(signaling_state_, exception_state))
    return nullptr;

  DCHECK(track);

  bool is_local_stream_track = false;
  for (const auto& local_stream : getLocalStreams()) {
    if (local_stream->getTracks().Contains(track)) {
      is_local_stream_track = true;
      break;
    }
  }
  if (!is_local_stream_track) {
    exception_state.ThrowDOMException(
        kSyntaxError, "No local stream is available for the track provided.");
    return nullptr;
  }

  RTCDTMFSender* dtmf_sender = RTCDTMFSender::Create(
      GetExecutionContext(), peer_handler_.get(), track, exception_state);
  if (exception_state.HadException())
    return nullptr;
  return dtmf_sender;
}

void RTCPeerConnection::close() {
  if (signaling_state_ == RTCPeerConnection::kSignalingStateClosed)
    return;

  CloseInternal();
}

void RTCPeerConnection::OnStreamAddTrack(MediaStream* stream,
                                         MediaStreamTrack* track) {
  DCHECK(track);
  DCHECK(track->Component());
  // Insert if not already present.
  tracks_.insert(track->Component(), track);
  CreateMissingSendersForStream(stream);
}

void RTCPeerConnection::OnStreamRemoveTrack(MediaStream* stream,
                                            MediaStreamTrack* track) {
  // Remove any senders that was removed from the lower layers as a result.
  RemoveUnusedSenders();
  // Don't remove |track| from |tracks_|, it may be referenced by another
  // component. |tracks_| uses weak members and will automatically have |track|
  // removed if destroyed.
}

void RTCPeerConnection::NegotiationNeeded() {
  DCHECK(!closed_);
  ScheduleDispatchEvent(Event::Create(EventTypeNames::negotiationneeded));
}

void RTCPeerConnection::DidGenerateICECandidate(
    scoped_refptr<WebRTCICECandidate> web_candidate) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  DCHECK(web_candidate);
  RTCIceCandidate* ice_candidate =
      RTCIceCandidate::Create(std::move(web_candidate));
  ScheduleDispatchEvent(
      RTCPeerConnectionIceEvent::Create(false, false, ice_candidate));
}

void RTCPeerConnection::DidChangeSignalingState(SignalingState new_state) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  ChangeSignalingState(new_state);
}

void RTCPeerConnection::DidChangeICEGatheringState(
    ICEGatheringState new_state) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  ChangeIceGatheringState(new_state);
}

void RTCPeerConnection::DidChangeICEConnectionState(
    ICEConnectionState new_state) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  ChangeIceConnectionState(new_state);
}

void RTCPeerConnection::DidAddRemoteTrack(
    std::unique_ptr<WebRTCRtpReceiver> web_rtp_receiver) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());
  if (signaling_state_ == kSignalingStateClosed)
    return;
  HeapVector<Member<MediaStream>> streams;
  WebVector<WebMediaStream> web_streams = web_rtp_receiver->Streams();
  streams.ReserveCapacity(web_streams.size());
  for (const WebMediaStream& web_stream : web_streams) {
    MediaStream* stream = getRemoteStream(web_stream);
    if (!stream) {
      // This is a new stream that we need to create.
      // Get or create audio tracks.
      WebVector<WebMediaStreamTrack> audio_web_tracks;
      web_stream.AudioTracks(audio_web_tracks);
      MediaStreamTrackVector audio_tracks;
      audio_tracks.ReserveCapacity(audio_web_tracks.size());
      for (const WebMediaStreamTrack& audio_web_track : audio_web_tracks) {
        MediaStreamTrack* audio_track = tracks_.at(audio_web_track);
        if (!audio_track) {
          audio_track =
              MediaStreamTrack::Create(GetExecutionContext(), audio_web_track);
          tracks_.insert(audio_track->Component(), audio_track);
        }
        audio_tracks.push_back(audio_track);
      }
      // Get or create video tracks.
      WebVector<WebMediaStreamTrack> video_web_tracks;
      web_stream.VideoTracks(video_web_tracks);
      MediaStreamTrackVector video_tracks;
      video_tracks.ReserveCapacity(video_web_tracks.size());
      for (const WebMediaStreamTrack& video_web_track : video_web_tracks) {
        MediaStreamTrack* video_track = tracks_.at(video_web_track);
        if (!video_track) {
          video_track =
              MediaStreamTrack::Create(GetExecutionContext(), video_web_track);
          tracks_.insert(video_track->Component(), video_track);
        }
        video_tracks.push_back(video_track);
      }
      // Create stream with tracks.
      stream = MediaStream::Create(GetExecutionContext(), web_stream,
                                   audio_tracks, video_tracks);
      stream->RegisterObserver(this);
      ScheduleDispatchEvent(
          MediaStreamEvent::Create(EventTypeNames::addstream, stream));
    } else {
      // The stream already exists. Because the blink stream is wired up to
      // reflect when web tracks are added to the corresponding web stream, the
      // receiver's track will already have a blink track created for it and
      // added to the blink stream. Find it and add it to |tracks_| so that the
      // RTCPeerConnection knows of its existence.
      // TODO(hbos): This wiring is problematic since it assumes the blink track
      // should always be created. If the track already exists (on some other
      // stream or receiver) we will end up with multiple blink tracks for the
      // same component. When a web track is added to the web stream, we need to
      // check if a blink track already exists for it by querying the
      // RTCPeerConnection. https://crbug.com/769743
      MediaStreamTrack* receiver_track = nullptr;
      for (const auto& track : stream->getTracks()) {
        if (track->Component() == web_rtp_receiver->Track()) {
          receiver_track = track;
          break;
        }
      }
      DCHECK(receiver_track);
      tracks_.insert(receiver_track->Component(), receiver_track);
    }
    streams.push_back(stream);
  }
  DCHECK(FindReceiver(*web_rtp_receiver) == rtp_receivers_.end());
  MediaStreamTrack* track = GetTrack(web_rtp_receiver->Track());
  if (!track) {
    // Receiver with track, without a stream. May be created by Unified Plan.
    track = MediaStreamTrack::Create(GetExecutionContext(),
                                     web_rtp_receiver->Track());
  }
  RTCRtpReceiver* rtp_receiver =
      new RTCRtpReceiver(std::move(web_rtp_receiver), track, streams);
  rtp_receivers_.push_back(rtp_receiver);
  ScheduleDispatchEvent(
      new RTCTrackEvent(rtp_receiver, rtp_receiver->track(), streams));
}

void RTCPeerConnection::DidRemoveRemoteTrack(
    std::unique_ptr<WebRTCRtpReceiver> web_rtp_receiver) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());

  WebVector<WebMediaStream> web_streams = web_rtp_receiver->Streams();
  auto it = FindReceiver(*web_rtp_receiver);
  DCHECK(it != rtp_receivers_.end());
  RTCRtpReceiver* rtp_receiver = *it;
  MediaStreamTrack* track = rtp_receiver->track();
  rtp_receivers_.erase(it);

  // End streams no longer in use and fire "removestream" events. This behavior
  // is no longer in the spec.
  for (const WebMediaStream& web_stream : web_streams) {
    MediaStreamDescriptor* stream_descriptor = web_stream;
    DCHECK(stream_descriptor->Client());
    MediaStream* stream =
        static_cast<MediaStream*>(stream_descriptor->Client());

    // The track should already have been removed from the stream thanks to
    // wiring listening to the webrtc layer stream. This should make sure the
    // "removetrack" event fires.
    DCHECK(!stream->getTracks().Contains(track));

    // Was this the last usage of the stream? Remove from remote streams.
    if (!getRemoteStreamUsageCount(web_stream)) {
      // TODO(hbos): The stream should already have ended by being empty, no
      // need for |StreamEnded|.
      stream->StreamEnded();
      stream->UnregisterObserver(this);
      ScheduleDispatchEvent(
          MediaStreamEvent::Create(EventTypeNames::removestream, stream));
    }
  }

  // Mute track and fire "onmute" if not already muted.
  track->Component()->Source()->SetReadyState(
      MediaStreamSource::kReadyStateMuted);
}

void RTCPeerConnection::DidAddRemoteDataChannel(
    WebRTCDataChannelHandler* handler) {
  DCHECK(!closed_);
  DCHECK(GetExecutionContext()->IsContextThread());

  if (signaling_state_ == kSignalingStateClosed)
    return;

  RTCDataChannel* channel =
      RTCDataChannel::Create(GetExecutionContext(), base::WrapUnique(handler));
  ScheduleDispatchEvent(RTCDataChannelEvent::Create(EventTypeNames::datachannel,
                                                    false, false, channel));
  has_data_channels_ = true;
}

void RTCPeerConnection::ReleasePeerConnectionHandler() {
  if (stopped_)
    return;

  stopped_ = true;
  ice_connection_state_ = kICEConnectionStateClosed;
  signaling_state_ = kSignalingStateClosed;

  dispatch_scheduled_event_runner_->Stop();

  peer_handler_.reset();

  connection_handle_for_scheduler_.reset();
}

void RTCPeerConnection::ClosePeerConnection() {
  DCHECK(signaling_state_ != RTCPeerConnection::kSignalingStateClosed);
  CloseInternal();
}

const AtomicString& RTCPeerConnection::InterfaceName() const {
  return EventTargetNames::RTCPeerConnection;
}

ExecutionContext* RTCPeerConnection::GetExecutionContext() const {
  return PausableObject::GetExecutionContext();
}

void RTCPeerConnection::Pause() {
  dispatch_scheduled_event_runner_->Pause();
}

void RTCPeerConnection::Unpause() {
  dispatch_scheduled_event_runner_->Unpause();
}

void RTCPeerConnection::ContextDestroyed(ExecutionContext*) {
  ReleasePeerConnectionHandler();
}

void RTCPeerConnection::ChangeSignalingState(SignalingState signaling_state) {
  if (signaling_state_ != kSignalingStateClosed) {
    signaling_state_ = signaling_state;
    ScheduleDispatchEvent(Event::Create(EventTypeNames::signalingstatechange));
  }
}

void RTCPeerConnection::ChangeIceGatheringState(
    ICEGatheringState ice_gathering_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed) {
    ScheduleDispatchEvent(
        Event::Create(EventTypeNames::icegatheringstatechange),
        WTF::Bind(&RTCPeerConnection::SetIceGatheringState,
                  WrapPersistent(this), ice_gathering_state));
    if (ice_gathering_state == kICEGatheringStateComplete) {
      // If ICE gathering is completed, generate a null ICE candidate, to
      // signal end of candidates.
      ScheduleDispatchEvent(
          RTCPeerConnectionIceEvent::Create(false, false, nullptr));
    }
  }
}

bool RTCPeerConnection::SetIceGatheringState(
    ICEGatheringState ice_gathering_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed &&
      ice_gathering_state_ != ice_gathering_state) {
    ice_gathering_state_ = ice_gathering_state;
    return true;
  }
  return false;
}

void RTCPeerConnection::ChangeIceConnectionState(
    ICEConnectionState ice_connection_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed) {
    ScheduleDispatchEvent(
        Event::Create(EventTypeNames::iceconnectionstatechange),
        WTF::Bind(&RTCPeerConnection::SetIceConnectionState,
                  WrapPersistent(this), ice_connection_state));
  }
}

bool RTCPeerConnection::SetIceConnectionState(
    ICEConnectionState ice_connection_state) {
  if (ice_connection_state_ != kICEConnectionStateClosed &&
      ice_connection_state_ != ice_connection_state) {
    ice_connection_state_ = ice_connection_state;
    if (ice_connection_state_ == kICEConnectionStateConnected)
      RecordRapporMetrics();

    return true;
  }
  return false;
}

void RTCPeerConnection::CloseInternal() {
  DCHECK(signaling_state_ != RTCPeerConnection::kSignalingStateClosed);
  peer_handler_->Stop();
  closed_ = true;

  ChangeIceConnectionState(kICEConnectionStateClosed);
  ChangeSignalingState(kSignalingStateClosed);
  Document* document = ToDocument(GetExecutionContext());
  HostsUsingFeatures::CountAnyWorld(
      *document, HostsUsingFeatures::Feature::kRTCPeerConnectionUsed);

  connection_handle_for_scheduler_.reset();
}

void RTCPeerConnection::ScheduleDispatchEvent(Event* event) {
  ScheduleDispatchEvent(event, BoolFunction());
}

void RTCPeerConnection::ScheduleDispatchEvent(Event* event,
                                              BoolFunction setup_function) {
  scheduled_events_.push_back(
      new EventWrapper(event, std::move(setup_function)));

  dispatch_scheduled_event_runner_->RunAsync();
}

void RTCPeerConnection::DispatchScheduledEvent() {
  if (stopped_)
    return;

  HeapVector<Member<EventWrapper>> events;
  events.swap(scheduled_events_);

  HeapVector<Member<EventWrapper>>::iterator it = events.begin();
  for (; it != events.end(); ++it) {
    if ((*it)->Setup()) {
      DispatchEvent((*it)->event_.Release());
    }
  }

  events.clear();
}

void RTCPeerConnection::RecordRapporMetrics() {
  Document* document = ToDocument(GetExecutionContext());
  for (const auto& component : tracks_.Keys()) {
    switch (component->Source()->GetType()) {
      case MediaStreamSource::kTypeAudio:
        HostsUsingFeatures::CountAnyWorld(
            *document, HostsUsingFeatures::Feature::kRTCPeerConnectionAudio);
        break;
      case MediaStreamSource::kTypeVideo:
        HostsUsingFeatures::CountAnyWorld(
            *document, HostsUsingFeatures::Feature::kRTCPeerConnectionVideo);
        break;
      default:
        NOTREACHED();
    }
  }

  if (has_data_channels_)
    HostsUsingFeatures::CountAnyWorld(
        *document, HostsUsingFeatures::Feature::kRTCPeerConnectionDataChannel);
}

void RTCPeerConnection::Trace(blink::Visitor* visitor) {
  visitor->Trace(tracks_);
  visitor->Trace(rtp_senders_);
  visitor->Trace(rtp_receivers_);
  visitor->Trace(dispatch_scheduled_event_runner_);
  visitor->Trace(scheduled_events_);
  EventTargetWithInlineData::Trace(visitor);
  PausableObject::Trace(visitor);
  MediaStreamObserver::Trace(visitor);
}

int RTCPeerConnection::PeerConnectionCount() {
  return InstanceCounters::CounterValue(
      InstanceCounters::kRTCPeerConnectionCounter);
}

int RTCPeerConnection::PeerConnectionCountLimit() {
  return kMaxPeerConnections;
}

}  // namespace blink
