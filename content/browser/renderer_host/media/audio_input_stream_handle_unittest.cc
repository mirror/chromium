// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/audio_input_stream_handle.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_handle.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sync_socket.h"
#include "media/audio/audio_input_delegate.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

using testing::StrictMock;
using testing::Mock;
using testing::Test;

const size_t kShmemSize = 1234;
const bool kInitiallyMuted = false;

class FakeAudioInputDelegate : public media::AudioInputDelegate {
 public:
  FakeAudioInputDelegate() {}

  ~FakeAudioInputDelegate() override {}

  int GetStreamId() override { return 0; };
  void OnRecordStream() override{};
  void OnSetVolume(double volume) override{};

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeAudioInputDelegate);
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
    input_stream_ = std::move(input_stream);
    client_request_ = std::move(client_request);
    Created();
  }

 private:
  media::mojom::AudioInputStreamPtr input_stream_;
  media::mojom::AudioInputStreamClientRequest client_request_;
};

class MockDeleter {
 public:
  MOCK_METHOD1(Delete, void(AudioInputStreamHandle*));
};

// Creates a fake delegate and saves the provided event handler in
// |event_handler_out|.
std::unique_ptr<media::AudioInputDelegate> CreateFakeDelegate(
    media::AudioInputDelegate::EventHandler** event_handler_out,
    media::AudioInputDelegate::EventHandler* event_handler) {
  *event_handler_out = event_handler;
  return std::make_unique<FakeAudioInputDelegate>();
}

}  // namespace

TEST(AudioInputStreamHandleTest, CreateStream) {
  base::MessageLoop l;
  StrictMock<MockRendererAudioInputStreamFactoryClient> client;
  mojom::RendererAudioInputStreamFactoryClientPtr client_ptr;
  mojo::Binding<mojom::RendererAudioInputStreamFactoryClient> client_binding(
      &client, mojo::MakeRequest(&client_ptr));
  StrictMock<MockDeleter> deleter;
  media::AudioInputDelegate::EventHandler* event_handler = nullptr;
  auto handle = std::make_unique<AudioInputStreamHandle>(
      std::move(client_ptr),
      base::BindOnce(&CreateFakeDelegate, &event_handler),
      base::BindOnce(&MockDeleter::Delete, base::Unretained(&deleter)));
  // Wait for |event_handler| to be set.
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(event_handler);

  base::SharedMemory shared_memory;
  shared_memory.CreateAndMapAnonymous(kShmemSize);
  auto local = std::make_unique<base::CancelableSyncSocket>();
  auto remote = std::make_unique<base::CancelableSyncSocket>();
  ASSERT_TRUE(
      base::CancelableSyncSocket::CreatePair(local.get(), remote.get()));
  event_handler->OnStreamCreated(/*stream_id, irrelevant*/ 0, &shared_memory,
                                 std::move(remote), kInitiallyMuted);

  EXPECT_CALL(client, Created());
  base::RunLoop().RunUntilIdle();
}

TEST(AudioInputStreamHandleTest,
     DestructClientBeforeCreationFinishes_CancelsStreamCreation) {
  base::MessageLoop l;
  StrictMock<MockRendererAudioInputStreamFactoryClient> client;
  mojom::RendererAudioInputStreamFactoryClientPtr client_ptr;
  mojo::Binding<mojom::RendererAudioInputStreamFactoryClient> client_binding(
      &client, mojo::MakeRequest(&client_ptr));
  StrictMock<MockDeleter> deleter;
  media::AudioInputDelegate::EventHandler* event_handler = nullptr;
  // Freed by DeleteArg below.
  auto* handle = new AudioInputStreamHandle(
      std::move(client_ptr),
      base::BindOnce(&CreateFakeDelegate, &event_handler),
      base::BindOnce(&MockDeleter::Delete, base::Unretained(&deleter)));
  // Wait for |event_handler| to be set.
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(event_handler);

  client_binding.Unbind();
  EXPECT_CALL(deleter, Delete(handle)).WillOnce(testing::DeleteArg<0>());
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter);
}

TEST(AudioInputStreamHandleTest,
     CreateStreamAndDisconnectClient_DestroysStream) {
  base::MessageLoop l;
  StrictMock<MockRendererAudioInputStreamFactoryClient> client;
  mojom::RendererAudioInputStreamFactoryClientPtr client_ptr;
  mojo::Binding<mojom::RendererAudioInputStreamFactoryClient> client_binding(
      &client, mojo::MakeRequest(&client_ptr));
  StrictMock<MockDeleter> deleter;
  media::AudioInputDelegate::EventHandler* event_handler = nullptr;
  // Freed by DeleteArg below.
  auto* handle = new AudioInputStreamHandle(
      std::move(client_ptr),
      base::BindOnce(&CreateFakeDelegate, &event_handler),
      base::BindOnce(&MockDeleter::Delete, base::Unretained(&deleter)));
  // Wait for |event_handler| to be set.
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(event_handler);

  base::SharedMemory shared_memory;
  shared_memory.CreateAndMapAnonymous(kShmemSize);
  auto local = std::make_unique<base::CancelableSyncSocket>();
  auto remote = std::make_unique<base::CancelableSyncSocket>();
  ASSERT_TRUE(
      base::CancelableSyncSocket::CreatePair(local.get(), remote.get()));
  event_handler->OnStreamCreated(/*stream_id, irrelevant*/ 0, &shared_memory,
                                 std::move(remote), kInitiallyMuted);

  EXPECT_CALL(client, Created());
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&client);

  client_binding.Unbind();
  EXPECT_CALL(deleter, Delete(handle)).WillOnce(testing::DeleteArg<0>());
  base::RunLoop().RunUntilIdle();
  Mock::VerifyAndClear(&deleter);
}

}  // namespace content
