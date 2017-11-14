// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "services/audio/audio_service.h"
#include "services/audio/audio_system_info.h"
#include "services/audio/in_process_audio_manager_accessor.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;

namespace audio {

class AudioServiceLifetimeTest : public testing::Test {
 public:
  enum { kQuitTimeoutMs = 10 };

  AudioServiceLifetimeTest() {}
  ~AudioServiceLifetimeTest() override {}

  void SetUp() override {
    // we are not using a separate audio thred
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(false));
    std::unique_ptr<AudioService> service_impl = std::make_unique<AudioService>(
        std::make_unique<InProcessAudioManagerAccessor>(audio_manager_.get()),
        base::TimeDelta::FromMilliseconds(kQuitTimeoutMs));
    service_ = service_impl.get();
    connector_factory_ =
        std::make_unique<service_manager::TestConnectorFactory>(
            std::move(service_impl));
    connector_ = connector_factory_->CreateConnector();
  }

  void TearDown() override { audio_manager_->Shutdown(); }

 protected:
  void WaitForQuitRequestTimeout() {
    base::RunLoop wait_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, wait_loop.QuitClosure(),
        base::TimeDelta::FromMilliseconds(kQuitTimeoutMs * 2));
    wait_loop.Run();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;
  AudioService* service_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioServiceLifetimeTest);
};

TEST_F(AudioServiceLifetimeTest, ServiceNotQuitWhenClientConnected) {
  StrictMock<base::MockCallback<base::Closure>> quit_request;
  service_->SetQuitClosureForTesting(quit_request.Get());
  EXPECT_CALL(quit_request, Run()).Times(0);

  mojom::AudioSystemInfoPtr info;
  connector_->BindInterface(mojom::kServiceName, &info);
  EXPECT_TRUE(info.is_bound());

  WaitForQuitRequestTimeout();
  EXPECT_TRUE(info.is_bound());
}

TEST_F(AudioServiceLifetimeTest,
       ServiceQuitAfterTimeoutWhenClientDisconnected) {
  base::RunLoop wait_loop;
  service_->SetQuitClosureForTesting(wait_loop.QuitClosure());
  mojom::AudioSystemInfoPtr info;
  connector_->BindInterface(mojom::kServiceName, &info);

  StrictMock<base::MockCallback<base::Closure>> execute_before_timeout;
  EXPECT_CALL(execute_before_timeout, Run()).Times(1);

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, execute_before_timeout.Get(),
      base::TimeDelta::FromMilliseconds(kQuitTimeoutMs / 2));

  info.reset();
  wait_loop.Run();
}

TEST_F(AudioServiceLifetimeTest,
       ServiceNotQuitWhenAnotherClientQuicklyConnects) {
  StrictMock<base::MockCallback<base::Closure>> quit_request;
  service_->SetQuitClosureForTesting(quit_request.Get());
  EXPECT_CALL(quit_request, Run()).Times(0);

  mojom::AudioSystemInfoPtr info1;
  connector_->BindInterface(mojom::kServiceName, &info1);
  EXPECT_TRUE(info1.is_bound());

  info1.reset();

  mojom::AudioSystemInfoPtr info2;
  connector_->BindInterface(mojom::kServiceName, &info2);
  EXPECT_TRUE(info2.is_bound());

  WaitForQuitRequestTimeout();
  EXPECT_TRUE(info2.is_bound());
}

TEST_F(AudioServiceLifetimeTest, ServiceNotQuitWhenOneClientRemainsConnected) {
  mojom::AudioSystemInfoPtr info1;
  mojom::AudioSystemInfoPtr info2;

  StrictMock<base::MockCallback<base::Closure>> quit_request;
  service_->SetQuitClosureForTesting(quit_request.Get());
  EXPECT_CALL(quit_request, Run()).Times(0);

  connector_->BindInterface(mojom::kServiceName, &info1);
  EXPECT_TRUE(info1.is_bound());
  connector_->BindInterface(mojom::kServiceName, &info2);
  EXPECT_TRUE(info2.is_bound());

  WaitForQuitRequestTimeout();
  EXPECT_TRUE(info1.is_bound());
  EXPECT_TRUE(info2.is_bound());

  info1.reset();
  EXPECT_TRUE(info2.is_bound());

  WaitForQuitRequestTimeout();
  EXPECT_FALSE(info1.is_bound());
  EXPECT_TRUE(info2.is_bound());

  // Now disconnect the last client and wait for service quite request.
  base::RunLoop wait_loop;
  service_->SetQuitClosureForTesting(wait_loop.QuitClosure());
  info2.reset();
  wait_loop.Run();
}

}  // namespace audio
