// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/audio_input_delegate_impl.h"

#include <stdint.h>

#include <limits>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/sync_socket.h"
#include "build/build_config.h"
#include "content/browser/media/capture/audio_mirroring_manager.h"
#include "content/browser/renderer_host/media/audio_sync_reader.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_observer.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/audio/audio_output_controller.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/fake_audio_input_stream.h"
#include "media/audio/fake_audio_log_factory.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/gmock_callback_support.h"
#include "media/base/media_switches.h"
#include "media/base/user_input_monitor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Mock;
using ::testing::InSequence;
using ::testing::DoubleEq;
using ::testing::AtLeast;
using ::testing::NotNull;
using ::testing::StrictMock;
using ::testing::NiceMock;
using ::testing::Return;

namespace content {

namespace {

const int kRenderProcessId = 1;
const int kRenderFrameId = 5;
const int kStreamId = 50;
const double kNewVolume = 0.618;
const double kVolumeScale = 2.71828;

media::AudioParameters Params() {
  return media::AudioParameters::UnavailableDeviceParams();
}

class MockEventHandler : public media::AudioInputDelegate::EventHandler {
 public:
  void OnStreamCreated(int stream_id,
                       const base::SharedMemory*,
                       std::unique_ptr<base::CancelableSyncSocket>,
                       bool initially_muted) override {
    MockOnStreamCreated(stream_id, initially_muted);
  }

  MOCK_METHOD2(MockOnStreamCreated, void(int, bool));
  MOCK_METHOD2(OnMuted, void(int, bool));
  MOCK_METHOD1(OnStreamError, void(int));
};

class MockUserInputMonitor : public media::UserInputMonitor {
 public:
  MockUserInputMonitor() {}

  size_t GetKeyPressCount() const { return 0; }

  MOCK_METHOD0(StartKeyboardMonitoring, void());
  MOCK_METHOD0(StopKeyboardMonitoring, void());
};

media::AudioOutputStream* ExpectNoOutputStreamCreation(
    const media::AudioParameters&,
    const std::string&) {
  CHECK(false);
  return nullptr;
}

media::AudioInputStream* ExpectNoInputStreamCreation(
    const media::AudioParameters&,
    const std::string&) {
  CHECK(false);
  return nullptr;
}

class MockAudioInputStream : public media::AudioInputStream {
 public:
  MockAudioInputStream() {}
  ~MockAudioInputStream() override {}

  MOCK_METHOD0(Open, bool());
  MOCK_METHOD1(Start, void(AudioInputCallback*));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD0(Close, void());
  MOCK_METHOD0(GetMaxVolume, double());
  MOCK_METHOD1(SetVolume, void(double));
  MOCK_METHOD0(GetVolume, double());
  MOCK_METHOD1(SetAutomaticGainControl, bool(bool));
  MOCK_METHOD0(GetAutomaticGainControl, bool());
  MOCK_METHOD0(IsMuted, bool());
};

}  // namespace

class AudioInputDelegateTest : public testing::Test {
 public:
  AudioInputDelegateTest()
      : thread_bundle_(base::in_place),
        audio_manager_(std::make_unique<media::TestAudioThread>()),
        audio_system_(&audio_manager_),
        media_stream_manager_(&audio_system_, audio_manager_.GetTaskRunner()) {
    audio_manager_.SetMakeInputStreamCB(
        base::BindRepeating(&ExpectNoInputStreamCreation));
    audio_manager_.SetMakeOutputStreamCB(
        base::BindRepeating(&ExpectNoOutputStreamCreation));
  }

  ~AudioInputDelegateTest() {
    audio_manager_.Shutdown();
    thread_bundle_.reset();
  }

 protected:
  int Open(const std::string& device_id, const std::string& name) {
    int session_id = media_stream_manager_.audio_input_device_manager()->Open(
        MediaStreamDevice(MEDIA_DEVICE_AUDIO_CAPTURE, device_id, name));
    base::RunLoop().RunUntilIdle();
    return session_id;
  }

  std::unique_ptr<media::AudioInputDelegate> CreateDelegate(
      uint32_t shared_memory_count,
      int session_id) {
    return AudioInputDelegateImpl::Create(
        &event_handler_, &audio_manager_, AudioMirroringManager::GetInstance(),
        &user_input_monitor_, &media_stream_manager_,
        log_factory_.CreateAudioLog(
            media::AudioLogFactory::AudioComponent::AUDIO_INPUT_CONTROLLER),
#if defined(OS_CHROMEOS)
        KeyboardMicRegistration(),
#endif
        shared_memory_count, kStreamId, session_id, kRenderProcessId,
        kRenderFrameId, false, Params());
  }

  base::Optional<TestBrowserThreadBundle> thread_bundle_;
  media::MockAudioManager audio_manager_;
  media::AudioSystemImpl audio_system_;
  MediaStreamManager media_stream_manager_;
  NiceMock<MockUserInputMonitor> user_input_monitor_;
  StrictMock<MockEventHandler> event_handler_;
  media::FakeAudioLogFactory log_factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioInputDelegateTest);
};

TEST_F(AudioInputDelegateTest,
       CreateWithoutAuthorization_FailsDelegateCreation) {
  EXPECT_EQ(CreateDelegate(10, 0), nullptr);

  // Ensure |event_handler_| didn't get any notifications.
  base::RunLoop().RunUntilIdle();
}

TEST_F(AudioInputDelegateTest,
       CreateWithTooManySegments_FailsDelegateCreation) {
  int session_id = Open("default", "default");
  EXPECT_EQ(CreateDelegate(std::numeric_limits<uint32_t>::max(), session_id),
            nullptr);

  // Ensure |event_handler_| didn't get any notifications.
  base::RunLoop().RunUntilIdle();
}

TEST_F(AudioInputDelegateTest, CreateWebContentsCaptureStream) {
  int session_id = Open(base::StringPrintf("web-contents-media-stream://%d:%d",
                                           kRenderProcessId, kRenderFrameId),
                        "Web contents stream");
  EXPECT_NE(CreateDelegate(10, session_id), nullptr);
  base::RunLoop().RunUntilIdle();
}

media::AudioInputStream* MakeInputStreamCallback(
    media::AudioInputStream* stream,
    bool* created,
    const media::AudioParameters&,
    const std::string&) {
  CHECK(!*created);
  *created = true;
  return stream;
}

TEST_F(AudioInputDelegateTest, CreateOrdinaryCaptureStream) {
  int session_id = Open("default", "default");

  auto delegate = CreateDelegate(10, session_id);
  EXPECT_NE(delegate, nullptr);

  StrictMock<MockAudioInputStream> stream;
  EXPECT_CALL(stream, Open()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(stream, SetAutomaticGainControl(false))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(stream, IsMuted()).Times(1).WillOnce(Return(false));
  EXPECT_CALL(event_handler_, MockOnStreamCreated(kStreamId, false));
  bool created = false;
  audio_manager_.SetMakeInputStreamCB(
      base::BindRepeating(&MakeInputStreamCallback, &stream, &created));

  base::RunLoop().RunUntilIdle();
  delegate.reset();
  EXPECT_CALL(stream, Close()).Times(1);
  base::RunLoop().RunUntilIdle();
}

TEST_F(AudioInputDelegateTest, Record) {
  int session_id = Open("default", "default");

  auto delegate = CreateDelegate(10, session_id);
  EXPECT_NE(delegate, nullptr);

  StrictMock<MockAudioInputStream> stream;
  EXPECT_CALL(stream, Open()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(stream, SetAutomaticGainControl(false))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(stream, IsMuted()).Times(1).WillOnce(Return(false));
  EXPECT_CALL(event_handler_, MockOnStreamCreated(kStreamId, false));
  bool created = false;
  audio_manager_.SetMakeInputStreamCB(
      base::BindRepeating(&MakeInputStreamCallback, &stream, &created));

  base::RunLoop().RunUntilIdle();

  delegate->OnRecordStream();
  EXPECT_CALL(stream, Start(NotNull())).Times(1);
  base::RunLoop().RunUntilIdle();

  delegate.reset();
  EXPECT_CALL(stream, Stop()).Times(1);
  EXPECT_CALL(stream, Close()).Times(1);
  base::RunLoop().RunUntilIdle();
}

TEST_F(AudioInputDelegateTest, SetVolume) {
  int session_id = Open("default", "default");

  auto delegate = CreateDelegate(10, session_id);
  EXPECT_NE(delegate, nullptr);

  StrictMock<MockAudioInputStream> stream;
  EXPECT_CALL(stream, Open()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(stream, SetAutomaticGainControl(false))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(stream, IsMuted()).Times(1).WillOnce(Return(false));
  EXPECT_CALL(event_handler_, MockOnStreamCreated(kStreamId, false));
  bool created = false;
  audio_manager_.SetMakeInputStreamCB(
      base::BindRepeating(&MakeInputStreamCallback, &stream, &created));

  base::RunLoop().RunUntilIdle();

  delegate->OnSetVolume(kNewVolume);

  // Note: The AudioInputController is supposed to access the max volume of the
  // stream and map [0, 1] to [0, max_volume].
  EXPECT_CALL(stream, GetMaxVolume()).Times(1).WillOnce(Return(kVolumeScale));
  EXPECT_CALL(stream, SetVolume(DoubleEq(kNewVolume * kVolumeScale))).Times(1);
  base::RunLoop().RunUntilIdle();

  delegate.reset();
  EXPECT_CALL(stream, Close()).Times(1);
  base::RunLoop().RunUntilIdle();
}

}  // namespace content
