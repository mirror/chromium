// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/render_frame_audio_input_stream_factory.h"

#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_handle.h"
#include "base/run_loop.h"
#include "base/sync_socket.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/base/audio_parameters.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

using testing::Test;

const int kSessionId = 234;
const bool kAGC = false;
const uint32_t kSharedMemoryCount = 345;
const int kSampleFrequency = 44100;
const int kBitsPerSample = 16;
const int kSamplesPerBuffer = kSampleFrequency / 100;
const bool kInitiallyMuted = false;

media::AudioParameters GetTestAudioParameters() {
  return media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                media::CHANNEL_LAYOUT_MONO, kSampleFrequency,
                                kBitsPerSample, kSamplesPerBuffer);
}

class FakeAudioInputDelegate : public media::AudioInputDelegate {
 public:
  // |on_destruction| can be used to observe the destruction of the delegate.
  FakeAudioInputDelegate() {}

  ~FakeAudioInputDelegate() override {}

  int GetStreamId() override { return 0; };

  // Stream control:
  void OnRecordStream() override{};
  void OnSetVolume(double volume) override{};

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeAudioInputDelegate);
};

class FakeAudioInputStreamClient : public media::mojom::AudioInputStreamClient {
 public:
  void OnMutedStateChanged(bool is_muted) override {}
  void OnError() override {}
};

class MockRendererAudioInputStreamFactoryClient
    : public mojom::RendererAudioInputStreamFactoryClient {
 public:
  MOCK_METHOD0(Created, void());

  void StreamCreated(media::mojom::AudioInputStreamPtr input_stream,
                     media::mojom::AudioInputStreamClientRequest client_request,
                     mojo::ScopedSharedBufferHandle shared_buffer,
                     mojo::ScopedHandle socket_descriptor,
                     bool initially_muted) override {
    Created();
  }
};

// Creates a fake delegate and saves the provided event handler in
// |event_handler_out|.
std::unique_ptr<media::AudioInputDelegate> CreateFakeDelegate(
    media::AudioInputDelegate::EventHandler** event_handler_out,
    AudioInputDeviceManager::KeyboardMicRegistration keyboard_mic_registration,
    uint32_t shared_memory_count,
    int stream_id,
    int session_id,
    bool automatic_gain_control,
    const media::AudioParameters& parameters,
    media::AudioInputDelegate::EventHandler* event_handler) {
  *event_handler_out = event_handler;
  return std::make_unique<FakeAudioInputDelegate>();
}

}  // namespace

TEST(RenderFrameAudioInputStreamFactoryTest, CreateStream) {
  TestBrowserThreadBundle thread_bundle;
  mojom::RendererAudioInputStreamFactoryPtr factory_ptr;
  media::mojom::AudioInputStreamPtr stream_ptr;
  MockRendererAudioInputStreamFactoryClient client;
  mojom::RendererAudioInputStreamFactoryClientPtr client_ptr;
  auto audio_input_device_manager =
      base::MakeRefCounted<AudioInputDeviceManager>(nullptr);
  media::AudioInputDelegate::EventHandler* event_handler = nullptr;
  mojo::Binding<mojom::RendererAudioInputStreamFactoryClient> client_binding(
      &client, mojo::MakeRequest(&client_ptr));
  auto factory_handle = RenderFrameAudioInputStreamFactoryHandle::CreateFactory(
      base::BindRepeating(&CreateFakeDelegate, &event_handler),
      audio_input_device_manager.get(), mojo::MakeRequest(&factory_ptr));

  factory_ptr->CreateStream(std::move(client_ptr), kSessionId,
                            GetTestAudioParameters(), kAGC, kSharedMemoryCount);

  // Wait for delegate to be created and |event_handler| set.
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(event_handler);
  base::SharedMemory shared_memory;
  shared_memory.CreateAndMapAnonymous(1234);
  auto local = std::make_unique<base::CancelableSyncSocket>();
  auto remote = std::make_unique<base::CancelableSyncSocket>();
  ASSERT_TRUE(
      base::CancelableSyncSocket::CreatePair(local.get(), remote.get()));
  event_handler->OnStreamCreated(/*stream_id, irrelevant*/ 0, &shared_memory,
                                 std::move(remote), kInitiallyMuted);

  EXPECT_CALL(client, Created());
  base::RunLoop().RunUntilIdle();
}

TEST(RenderFrameAudioInputStreamFactoryTest, OutOfRangeSessionId_BadMessage) {
  // This test checks that we get a bad message if session_id is too large
  // to fit in an integer. This ensures that we don't overflow when casting the
  // int64_t to an int
  if (sizeof(int) >= sizeof(int64_t)) {
    // In this case, any int64_t would fit in an int, and the case we are
    // checking for is impossible.
    return;
  }

  int64_t session_id = std::numeric_limits<int>::max();
  ++session_id;

  bool got_bad_message = false;
  mojo::edk::SetDefaultProcessErrorCallback(
      base::Bind([](bool* got_bad_message,
                    const std::string& s) { *got_bad_message = true; },
                 &got_bad_message));

  TestBrowserThreadBundle thread_bundle;
  mojom::RendererAudioInputStreamFactoryPtr factory_ptr;
  media::mojom::AudioInputStreamPtr stream_ptr;
  MockRendererAudioInputStreamFactoryClient client;
  mojom::RendererAudioInputStreamFactoryClientPtr client_ptr;
  auto audio_input_device_manager =
      base::MakeRefCounted<AudioInputDeviceManager>(nullptr);
  media::AudioInputDelegate::EventHandler* event_handler = nullptr;
  mojo::Binding<mojom::RendererAudioInputStreamFactoryClient> client_binding(
      &client, mojo::MakeRequest(&client_ptr));
  auto factory_handle = RenderFrameAudioInputStreamFactoryHandle::CreateFactory(
      base::BindRepeating(&CreateFakeDelegate, &event_handler),
      audio_input_device_manager.get(), mojo::MakeRequest(&factory_ptr));

  factory_ptr->CreateStream(std::move(client_ptr), session_id,
                            GetTestAudioParameters(), kAGC, kSharedMemoryCount);

  EXPECT_FALSE(got_bad_message);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(got_bad_message);
}

}  // namespace content
