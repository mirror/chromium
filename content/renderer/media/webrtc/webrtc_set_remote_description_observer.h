// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/common/content_export.h"
#include "content/renderer/media/rtc_peer_connection_handler.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter.h"
#include "third_party/webrtc/api/rtpreceiverinterface.h"
#include "third_party/webrtc/api/setremotedescriptionobserver.h"
#include "third_party/webrtc/rtc_base/refcount.h"
#include "third_party/webrtc/rtc_base/refcountedobject.h"
#include "third_party/webrtc/rtc_base/scoped_ref_ptr.h"

namespace content {

// TODO(hbos): Update comments

// TODO(hbos): Write a design doc for this too? Documents complexity. Totally
// worth it. And helps reviewers. Link to design doc from code? Helps anyone
// reading the code. Also link to earlier design doc in the webrtc code.

// Include plan, e.g.
// 1. Observer interface and glue.
// 2. Replace RTCPeerConnectionHandler::OBserver stuff with implementation of
//    new interface.
// 3. Nuke other code paths, e.g. streams listening to stream events. Make sure
//    we don't need to listen for other reasons such as pc.close() terminating
//    remote streams or the like.

// TODO(hbos): Remove DCHECKs in receivers etc assuming state match. There are
// no such guarantees, ever, on the main thread.

// Describes an instance of a receiver being added. Because webrtc and content
// operate on different threads, webrtc objects may have been modified by the
// time this event is processed on the main thread.
struct CONTENT_EXPORT WebRtcReceiverState {
  WebRtcReceiverState(
      scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref,
      std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
          stream_refs);
  WebRtcReceiverState(const WebRtcReceiverState& other) = delete;
  WebRtcReceiverState(WebRtcReceiverState&& other);
  ~WebRtcReceiverState();

  scoped_refptr<webrtc::RtpReceiverInterface> receiver;
  // The receiver's track when this event occurred.
  std::unique_ptr<WebRtcMediaStreamTrackAdapterMap::AdapterRef> track_ref;
  // The receiver's associated set of streams when this event occurred.
  std::vector<std::unique_ptr<WebRtcMediaStreamAdapterMap::AdapterRef>>
      stream_refs;
};

// The content layer correspondent of webrtc::SetRemoteDescriptionObserver.
// An interface with callbacks for handling the result of SetRemoteDescription
// on the main thread.
class CONTENT_EXPORT WebRtcSetRemoteDescriptionObserverInterface
    : public base::RefCountedThreadSafe<
          WebRtcSetRemoteDescriptionObserverInterface> {
 public:
  // The relevant peer connection states as they were when the
  // SetRemoteDescription call completed. This is used instead of inspecting the
  // PeerConnection and other webrtc objects directly because they may have been
  // modified before we reach the main thread.
  struct States {
    States();
    States(const States& other) = delete;
    States(States&& other);
    ~States();

    States& operator=(const States& other) = delete;
    States& operator=(States&& other);

    // The receivers at the time of the event.
    std::vector<WebRtcReceiverState> receiver_states;
  };

  // Invoked asynchronously on the main thread after the SetRemoteDescription
  // completed on the webrtc signaling thread.
  virtual void OnSuccess(States states) = 0;
  virtual void OnFailure(webrtc::FailureReason failure_reason) = 0;

 protected:
  friend class base::RefCountedThreadSafe<
      WebRtcSetRemoteDescriptionObserverInterface>;
  virtual ~WebRtcSetRemoteDescriptionObserverInterface();
};

// The glue between webrtc and content layer observers listening to
// SetRemoteDescription. This observer listens on the webrtc signaling thread
// for the result of SetRemoteDescription, translates webrtc "state changes" to
// content "state changes" and jumps to the main thread for a
// WebRtcSetRemoteDescriptionObserverInterface to process the content layer
// "state changes".
class CONTENT_EXPORT WebRtcSetRemoteDescriptionObserverHandler
    : public rtc::RefCountedObject<webrtc::SetRemoteDescriptionObserver> {
 public:
  WebRtcSetRemoteDescriptionObserverHandler(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread,
      scoped_refptr<webrtc::PeerConnectionInterface> pc,
      scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map,
      scoped_refptr<WebRtcSetRemoteDescriptionObserverInterface> observer);
  ~WebRtcSetRemoteDescriptionObserverHandler() override;

  // webrtc::SetRemoteDescriptionObserver overrides.
  void OnSuccess(webrtc::SetRemoteDescriptionObserver::StateChanges
                     webrtc_states) override;
  void OnFailure(webrtc::FailureReason reason) override;

 private:
  void OnSuccessOnMainThread(
      WebRtcSetRemoteDescriptionObserverInterface::States content_states);
  void OnFailureOnMainThread(webrtc::FailureReason failure_reason);

  scoped_refptr<WebRtcMediaStreamTrackAdapterMap> track_adapter_map() const {
    return stream_adapter_map_->track_adapter_map();
  }

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<webrtc::PeerConnectionInterface> pc_;
  scoped_refptr<WebRtcMediaStreamAdapterMap> stream_adapter_map_;
  scoped_refptr<WebRtcSetRemoteDescriptionObserverInterface> observer_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_WEBRTC_SET_REMOTE_DESCRIPTION_OBSERVER_H_
