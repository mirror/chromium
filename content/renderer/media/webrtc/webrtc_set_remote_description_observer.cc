// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_set_remote_description_observer.h"

#include "base/logging.h"

namespace content {

WebRtcReceiverAdded::WebRtcReceiverAdded(
    scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref,
    std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
        stream_refs)
    : receiver(std::move(receiver)),
      track_ref(std::move(track_ref)),
      stream_refs(std::move(stream_refs)) {}

WebRtcReceiverAdded::WebRtcReceiverAdded(WebRtcReceiverAdded&& other)
    : receiver(std::move(other.receiver)),
      track_ref(std::move(other.track_ref)),
      stream_refs(std::move(other.stream_refs)) {}

WebRtcReceiverAdded::~WebRtcReceiverAdded() {}

WebRtcReceiverRemoved::WebRtcReceiverRemoved(
    scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref)
    : receiver(std::move(receiver)), track_ref(std::move(track_ref)) {}

WebRtcReceiverRemoved::WebRtcReceiverRemoved(WebRtcReceiverRemoved&& other)
    : receiver(std::move(other.receiver)),
      track_ref(std::move(other.track_ref)) {}

WebRtcReceiverRemoved::~WebRtcReceiverRemoved() {}

WebRtcSetRemoteDescriptionObserverInterface::StateChanges::StateChanges() {}

WebRtcSetRemoteDescriptionObserverInterface::StateChanges::StateChanges(
    StateChanges&& other)
    : receivers_added(std::move(other.receivers_added)),
      receivers_removed(std::move(other.receivers_removed)) {}

WebRtcSetRemoteDescriptionObserverInterface::StateChanges::~StateChanges() {}

WebRtcSetRemoteDescriptionObserverInterface::StateChanges&
WebRtcSetRemoteDescriptionObserverInterface::StateChanges::operator=(
    StateChanges&& other) {
  receivers_added = std::move(other.receivers_added);
  receivers_removed = std::move(other.receivers_removed);
  return *this;
}

WebRtcSetRemoteDescriptionObserverInterface::
    ~WebRtcSetRemoteDescriptionObserverInterface() {}

WebRtcSetRemoteDescriptionObserverHandler::
    WebRtcSetRemoteDescriptionObserverHandler(
        scoped_refptr<base::SingleThreadTaskRunner> main_thread,
        scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map,
        scoped_refptr<WebRtcSetRemoteDescriptionObserverInterface> observer)
    : main_thread_(std::move(main_thread)),
      stream_adapter_map_(std::move(stream_adapter_map)),
      observer_(std::move(observer)) {}

WebRtcSetRemoteDescriptionObserverHandler::
    ~WebRtcSetRemoteDescriptionObserverHandler() {}

void WebRtcSetRemoteDescriptionObserverHandler::OnSuccess(
    webrtc::SetRemoteDescriptionObserver::StateChanges webrtc_state_changes) {
  DCHECK(!main_thread_->BelongsToCurrentThread());

  WebRtcSetRemoteDescriptionObserverInterface::StateChanges
      content_state_changes;

  for (const auto& webrtc_receiver_added :
       webrtc_state_changes.receivers_added) {
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref =
        track_adapter_map()->GetOrCreateRemoteTrackAdapter(
            webrtc_receiver_added.receiver->track().get());
    std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
        stream_refs;
    for (const auto& stream : webrtc_receiver_added.streams) {
      stream_refs.push_back(
          stream_adapter_map_->GetOrCreateRemoteStreamAdapter(stream.get()));
    }
    content_state_changes.receivers_added.push_back(
        WebRtcReceiverAdded(webrtc_receiver_added.receiver.get(),
                            std::move(track_ref), std::move(stream_refs)));
  }

  for (const auto& webrtc_receiver_removed :
       webrtc_state_changes.receivers_removed) {
    std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref =
        track_adapter_map()->GetOrCreateRemoteTrackAdapter(
            webrtc_receiver_removed->track().get());
    content_state_changes.receivers_removed.push_back(WebRtcReceiverRemoved(
        webrtc_receiver_removed.get(), std::move(track_ref)));
  }

  main_thread_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &WebRtcSetRemoteDescriptionObserverHandler::OnSuccessOnMainThread,
          this, base::Passed(&content_state_changes)));
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
    WebRtcSetRemoteDescriptionObserverInterface::StateChanges state_changes) {
  DCHECK(main_thread_->BelongsToCurrentThread());
  observer_->OnSuccess(std::move(state_changes));
}

void WebRtcSetRemoteDescriptionObserverHandler::OnFailureOnMainThread(
    webrtc::FailureReason failure_reason) {
  DCHECK(main_thread_->BelongsToCurrentThread());
  observer_->OnFailure(std::move(failure_reason));
}

}  // namespace content
