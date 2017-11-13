// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "media/audio/audio_system_test_util.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "services/audio/audio_service.h"
#include "services/audio/audio_system_info.h"
#include "services/audio/in_process_audio_manager_accessor.h"
// #include "base/test/scoped_task_environment.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
//#include "services/service_manager/public/cpp/service_context.h"
//#include "services/service_manager/public/cpp/service_test.h"
//#include "services/service_manager/public/interfaces/service_factory.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace audio {

class AudioServiceLifetimeTest : public testing::Test {
 public:
  AudioServiceLifetimeTest() {}
  ~AudioServiceLifetimeTest() override {}

  void SetUp() override {
    // we are not using a separate audio thred
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(false));
    //   std::unique_ptr<AudioService> service_impl =
    //   std::make_unique<AudioService>(
    //       std::make_unique<InProcessAudioManagerAccessor>(
    //           audio_manager_.get()),
         base::TimeDelta::FromMilliseconds(1000));
         service_ = service_impl.get();
         connector_factory_ =
             std::make_unique<service_manager::TestConnectorFactory>(
                 std::make_unique<AudioService>(
                     std::make_unique<InProcessAudioManagerAccessor>(
                         audio_manager_.get()),
                     bas::TimeDelta::FromMilliseconds(100)));
         connector_ = connector_factory_->CreateConnector();
         //   connector_->BindInterface(mojom::kServiceName,
         //   &factory_provider_);
  }

  void TearDown() override { audio_manager_->Shutdown(); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  //   AudioService* service_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;
  // Owning accessor / MockAudioManagerAccessor
 private:
  DISALLOW_COPY_AND_ASSIGN(AudioServiceLifetimeTest);
};

TEST_F(AudioServiceLifetimeTest, ServiceNotQuitWhenClientConnected) {
  //   base::RunLoop wait_loop;
  mojom::AudioSystemInfoPtr info;
  connector_->BindInterface(mojom::kServiceName, &info);
  //   service_impl_->SetFactoryProviderClientDisconnectedObserver(
  //       wait_loop.QuitClosure());
  //   info.reset();
  //   wait_loop.RunUntilIdle();
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(info.is_bound());
}

}  // namespace audio
