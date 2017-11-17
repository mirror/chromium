// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_set_remote_description_observer.h"

#include "base/logging.h"

namespace content {

WebRtcReceiverState::WebRtcReceiverState(
    scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref,
    std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
        stream_refs)
    : receiver(std::move(receiver)),
      track_ref(std::move(track_ref)),
      stream_refs(std::move(stream_refs)) {}

WebRtcReceiverState::WebRtcReceiverState(WebRtcReceiverState&& other)
    : receiver(std::move(other.receiver)),
      track_ref(std::move(other.track_ref)),
      stream_refs(std::move(other.stream_refs)) {}

WebRtcReceiverState::~WebRtcReceiverState() {}

WebRtcSetRemoteDescriptionObserverInterface::States::States() {}

WebRtcSetRemoteDescriptionObserverInterface::States::States(States&& other)
    : receiver_states(std::move(other.receiver_states)) {}

WebRtcSetRemoteDescriptionObserverInterface::States::~States() {}

WebRtcSetRemoteDescriptionObserverInterface::States&
WebRtcSetRemoteDescriptionObserverInterface::States::operator=(States&& other) {
  receiver_states = std::move(other.receiver_states);
  return *this;
}

WebRtcSetRemoteDescriptionObserverInterface::
    ~WebRtcSetRemoteDescriptionObserverInterface() {}

WebRtcSetRemoteDescriptionObserverHandler::
    WebRtcSetRemoteDescriptionObserverHandler(
        scoped_refptr<base::SingleThreadTaskRunner> main_thread,
        scoped_refptr<webrtc::PeerConnectionInterface> pc,
        scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map,
        scoped_refptr<WebRtcSetRemoteDescriptionObserverInterface> observer)
    : main_thread_(std::move(main_thread)),
      pc_(std::move(pc)),
      stream_adapter_map_(std::move(stream_adapter_map)),
      observer_(std::move(observer)) {}

WebRtcSetRemoteDescriptionObserverHandler::
    ~WebRtcSetRemoteDescriptionObserverHandler() {}

void WebRtcSetRemoteDescriptionObserverHandler::OnSuccess() {
  DCHECK(!main_thread_->BelongsToCurrentThread());

  WebRtcSetRemoteDescriptionObserverInterface::States content_states;

  for (const auto& webrtc_receiver : pc_->GetReceivers()) {
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref =
        track_adapter_map()->GetOrCreateRemoteTrackAdapter(
            webrtc_receiver->track().get());
    std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
        stream_refs;
    for (const auto& stream : webrtc_receiver->streams()) {
      stream_refs.push_back(
          stream_adapter_map_->GetOrCreateRemoteStreamAdapter(stream.get()));
    }
    content_states.receiver_states.push_back(WebRtcReceiverState(
        webrtc_receiver.get(), std::move(track_ref), std::move(stream_refs)));
  }

  main_thread_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &WebRtcSetRemoteDescriptionObserverHandler::OnSuccessOnMainThread,
          this, base::Passed(&content_states)));
}

void WebRtcSetRemoteDescriptionObserverHandler::OnFailure(
    webrtc::FailureReason reason) {
  DCHECK(!main_thread_->BelongsToCurrentThread());
  main_thread_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &WebRtcSetRemoteDescriptionObserverHandler::OnFailureOnMainThread,
          this, base::Passed(&reason)));
}

void WebRtcSetRemoteDescriptionObserverHandler::OnSuccessOnMainThread(
    WebRtcSetRemoteDescriptionObserverInterface::States states) {
  DCHECK(main_thread_->BelongsToCurrentThread());
  observer_->OnSuccess(std::move(states));
}

void WebRtcSetRemoteDescriptionObserverHandler::OnFailureOnMainThread(
    webrtc::FailureReason failure_reason) {
  DCHECK(main_thread_->BelongsToCurrentThread());
  observer_->OnFailure(std::move(failure_reason));
}

}  // namespace content
