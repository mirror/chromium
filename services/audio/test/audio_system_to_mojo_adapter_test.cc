// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_impl.h"
//#include "base/message_loop/message_loop.h"
#include "media/audio/audio_system_tester.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/audio/audio_service.h"
#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"

#include "base/debug/stack_trace.h"

namespace audio {

class ServiceTestClient : public service_manager::test::ServiceTestClient,
                          public service_manager::mojom::ServiceFactory {
 public:
  ServiceTestClient(service_manager::test::ServiceTest* test,
                    media::AudioManager* audio_manager)
      : service_manager::test::ServiceTestClient(test),
        audio_manager_(audio_manager) {
    DCHECK(audio_manager);
    DLOG(ERROR) << "***ServiceTestClient constructor";
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::Bind(&ServiceTestClient::Create, base::Unretained(this)));
  }

 protected:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void CreateService(service_manager::mojom::ServiceRequest request,
                     const std::string& name) override {
    if (name == mojom::kServiceName) {
      DLOG(ERROR) << "***CreateService";
      audio_service_context_ =
          std::make_unique<service_manager::ServiceContext>(
              std::make_unique<AudioService>(
                  std::make_unique<AudioService::AudioManagerAccessor>(
                      audio_manager_)),
              std::move(request));
      DLOG(ERROR) << "***CreateService end";
    }
  }

  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

 private:
  media::AudioManager* audio_manager_;
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  std::unique_ptr<service_manager::ServiceContext> audio_service_context_;
};

class AudioSystemToMojoAdapterTest : public service_manager::test::ServiceTest {
 public:
  AudioSystemToMojoAdapterTest() : ServiceTest("audio_unittests") {
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(true /*use_audio_thread_*/));
  }

  ~AudioSystemToMojoAdapterTest() override { audio_manager_->Shutdown(); }

 protected:
  // service_manager::test::ServiceTest:
  std::unique_ptr<service_manager::Service> CreateService() override {
    DLOG(ERROR) << "*** CreatService";
    return base::MakeUnique<ServiceTestClient>(this, audio_manager_.get());
  }

  void SetUp() override {
    ServiceTest::SetUp();
    audio_system_ = std::make_unique<AudioSystemToMojoAdapter>(connector());
    tester_ = std::make_unique<media::AudioSystemTester>(audio_manager_.get(),
                                                         audio_system_.get());
  }

  void TearDown() override {
    tester_.reset();
    audio_system_.reset();
    ServiceTest::TearDown();
  }

  // bool use_audio_thread_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  std::unique_ptr<media::AudioSystemTester> tester_;
};

#define AUDIO_SYSTEM_TEST(test_name)                \
  TEST_F(AudioSystemToMojoAdapterTest, test_name) { \
    tester_->Test##test_name();                     \
  }

AUDIO_SYSTEM_TEST(GetInputStreamParametersNormal);
AUDIO_SYSTEM_TEST(GetInputStreamParametersNoDevice);
AUDIO_SYSTEM_TEST(GetOutputStreamParameters);
AUDIO_SYSTEM_TEST(GetDefaultOutputStreamParameters);
AUDIO_SYSTEM_TEST(GetOutputStreamParametersForDefaultDeviceNoDevices);
AUDIO_SYSTEM_TEST(GetOutputStreamParametersForNonDefaultDeviceNoDevices);
AUDIO_SYSTEM_TEST(HasInputDevices);
AUDIO_SYSTEM_TEST(HasNoInputDevices);
AUDIO_SYSTEM_TEST(HasOutputDevices);
AUDIO_SYSTEM_TEST(HasNoOutputDevices);
AUDIO_SYSTEM_TEST(GetInputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_TEST(GetInputDeviceDescriptions);
AUDIO_SYSTEM_TEST(GetOutputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_TEST(GetOutputDeviceDescriptions);
AUDIO_SYSTEM_TEST(GetAssociatedOutputDeviceID);
AUDIO_SYSTEM_TEST(GetInputDeviceInfoNoAssociation);
AUDIO_SYSTEM_TEST(GetInputDeviceInfoWithAssociation);

#undef AUDIO_SYSTEM_TEST

// INSTANTIATE_TEST_CASE_P(, AudioSystemToMojoAdapterTest,
// testing::Values(false, true));

}  // namespace audio
