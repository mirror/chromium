// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_set_remote_description_observer.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/child_process.h"
#include "content/renderer/media/mock_peer_connection_impl.h"
#include "content/renderer/media/webrtc/mock_peer_connection_dependency_factory.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_adapter_map.h"
#include "content/renderer/media/webrtc/webrtc_media_stream_track_adapter_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/web/WebHeap.h"
#include "third_party/webrtc/api/peerconnectioninterface.h"
#include "third_party/webrtc/media/base/fakemediaengine.h"
#include "third_party/webrtc/pc/test/mock_peerconnection.h"

using ::testing::Return;

namespace content {

class WebRtcSetRemoteDescriptionObserverForTest
    : public WebRtcSetRemoteDescriptionObserverInterface {
 public:
  bool called() const { return result_.has_value(); }
  bool result() const { return *result_; }

  const WebRtcSetRemoteDescriptionObserverInterface::States& states() const {
    DCHECK(called() && result());
    return states_;
  }
  const webrtc::FailureReason& failure_reason() const {
    DCHECK(called() && !result());
    return failure_reason_;
  }

  // WebRtcSetRemoteDescriptionObserverInterface overrides.
  void OnSuccess(
      WebRtcSetRemoteDescriptionObserverInterface::States states) override {
    result_ = base::Optional<bool>(true);
    states_ = std::move(states);
  }

  void OnFailure(webrtc::FailureReason failure_reason) override {
    result_ = base::Optional<bool>(false);
    failure_reason_ = failure_reason;
  }

 private:
  ~WebRtcSetRemoteDescriptionObserverForTest() override {}

  base::Optional<bool> result_;
  WebRtcSetRemoteDescriptionObserverInterface::States states_;
  webrtc::FailureReason failure_reason_;
};

class WebRtcSetRemoteDescriptionObserverHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    media_engine_ = new cricket::FakeMediaEngine();       // not needed
    pc_factory_ = new webrtc::FakePeerConnectionFactory(  // not needed
        std::unique_ptr<cricket::MediaEngineInterface>(media_engine_)),
    pc_ = new webrtc::MockPeerConnection(pc_factory_.get());

    dependency_factory_.reset(new MockPeerConnectionDependencyFactory());
    main_thread_ = base::ThreadTaskRunnerHandle::Get();
    scoped_refptr<WebRtcMediaStreamAdapterMap> map =
        new WebRtcMediaStreamAdapterMap(
            dependency_factory_.get(),
            new WebRtcMediaStreamTrackAdapterMap(dependency_factory_.get()));
    observer_ = new WebRtcSetRemoteDescriptionObserverForTest();
    observer_handler_ = new WebRtcSetRemoteDescriptionObserverHandler(
        main_thread_, pc_, map, observer_);
  }

  void TearDown() override { blink::WebHeap::CollectAllGarbageForTesting(); }

  void InvokeOnSuccess() {
    base::WaitableEvent waitable_event(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&WebRtcSetRemoteDescriptionObserverHandlerTest::
                           InvokeOnSuccessOnSignalingThread,
                       base::Unretained(this),
                       base::Unretained(&waitable_event)));
    waitable_event.Wait();
    base::RunLoop().RunUntilIdle();
  }

  void InvokeOnFailure(webrtc::FailureReason failure_reason) {
    base::WaitableEvent waitable_event(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    dependency_factory_->GetWebRtcSignalingThread()->PostTask(
        FROM_HERE,
        base::BindOnce(&WebRtcSetRemoteDescriptionObserverHandlerTest::
                           InvokeOnFailureOnSignalingThread,
                       base::Unretained(this), base::Passed(&failure_reason),
                       base::Unretained(&waitable_event)));
    waitable_event.Wait();
    base::RunLoop().RunUntilIdle();
  }

 protected:
  void InvokeOnSuccessOnSignalingThread(base::WaitableEvent* waitable_event) {
    observer_handler_->OnSuccess();
    waitable_event->Signal();
  }

  void InvokeOnFailureOnSignalingThread(webrtc::FailureReason failure_reason,
                                        base::WaitableEvent* waitable_event) {
    observer_handler_->OnFailure(std::move(failure_reason));
    waitable_event->Signal();
  }

  // Message loop and child processes is needed for task queues and threading to
  // work, as is necessary to create tracks and adapters.
  base::MessageLoop message_loop_;
  ChildProcess child_process_;

  // |media_engine_| is actually owned by |pc_factory_|.
  cricket::FakeMediaEngine* media_engine_;  // not needed, owned by pc_factory_
  scoped_refptr<webrtc::FakePeerConnectionFactory>
      pc_factory_;  // not needed, owned by pc_
  scoped_refptr<webrtc::MockPeerConnection> pc_;

  std::unique_ptr<MockPeerConnectionDependencyFactory> dependency_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_;
  scoped_refptr<WebRtcSetRemoteDescriptionObserverForTest> observer_;
  scoped_refptr<WebRtcSetRemoteDescriptionObserverHandler> observer_handler_;

  std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers_;
};

TEST_F(WebRtcSetRemoteDescriptionObserverHandlerTest, OnSuccess) {
  scoped_refptr<MockWebRtcAudioTrack> added_track =
      MockWebRtcAudioTrack::Create("added_track");
  scoped_refptr<webrtc::MediaStreamInterface> added_stream(
      new rtc::RefCountedObject<MockMediaStream>("added_stream"));
  scoped_refptr<webrtc::RtpReceiverInterface> added_receiver(
      new rtc::RefCountedObject<FakeRtpReceiver>(
          added_track.get(),
          std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>(
              {added_stream.get()})));

  receivers_.push_back(added_receiver.get());
  EXPECT_CALL(*pc_, GetReceivers()).WillRepeatedly(Return(receivers_));

  InvokeOnSuccess();
  EXPECT_TRUE(observer_->called());
  EXPECT_TRUE(observer_->result());

  EXPECT_EQ(1u, observer_->states().receiver_states.size());
  const WebRtcReceiverState& receiver_state =
      observer_->states().receiver_states[0];
  EXPECT_EQ(added_receiver, receiver_state.receiver);
  EXPECT_EQ(added_track, receiver_state.track_ref->webrtc_track());
  EXPECT_EQ(1u, receiver_state.stream_refs.size());
  EXPECT_EQ(added_stream,
            receiver_state.stream_refs[0]->adapter().webrtc_stream());
}

TEST_F(WebRtcSetRemoteDescriptionObserverHandlerTest, OnFailure) {
  webrtc::FailureReason failure_reason("Oh noes!");
  InvokeOnFailure(std::move(failure_reason));
  EXPECT_TRUE(observer_->called());
  EXPECT_FALSE(observer_->result());
  EXPECT_EQ("Oh noes!", *observer_->failure_reason().message);
}

}  // namespace content
