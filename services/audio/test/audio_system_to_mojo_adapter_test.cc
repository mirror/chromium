// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/audio_system_tester.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/audio/audio_service.h"
#include "services/audio/audio_system_info.h"
#include "services/audio/in_process_audio_manager_accessor.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"

#include "media/audio/audio_device_description.h"

namespace audio {

class ServiceTestClient : public service_manager::test::ServiceTestClient,
                          public service_manager::mojom::ServiceFactory {
 public:
  class AudioThreadContext
      : public base::RefCountedThreadSafe<AudioThreadContext> {
   public:
    explicit AudioThreadContext(media::AudioManager* audio_manager)
        : audio_manager_(audio_manager) {}

    void CreateServiceOnAudioThread(
        service_manager::mojom::ServiceRequest request) {
      if (!audio_manager_->GetTaskRunner()->BelongsToCurrentThread()) {
        audio_manager_->GetTaskRunner()->PostTask(
            FROM_HERE,
            base::BindOnce(&AudioThreadContext::CreateServiceOnAudioThread,
                           this, std::move(request)));
        return;
      }
      DCHECK(!audio_service_context_);
      audio_service_context_ =
          std::make_unique<service_manager::ServiceContext>(
              std::make_unique<AudioService>(
                  std::make_unique<InProcessAudioManagerAccessor>(
                      audio_manager_),
                  base::TimeDelta()),
              std::move(request));
    }

    void QuitOnAudioThread() {
      if (!audio_manager_->GetTaskRunner()->BelongsToCurrentThread()) {
        audio_manager_->GetTaskRunner()->PostTask(
            FROM_HERE,
            base::BindOnce(&AudioThreadContext::QuitOnAudioThread, this));
        return;
      }
      audio_service_context_.reset();
    }

   private:
    friend class base::RefCountedThreadSafe<AudioThreadContext>;
    virtual ~AudioThreadContext(){};

    media::AudioManager* const audio_manager_;
    std::unique_ptr<service_manager::ServiceContext> audio_service_context_;
  };

  ServiceTestClient(service_manager::test::ServiceTest* test,
                    media::AudioManager* audio_manager)
      : service_manager::test::ServiceTestClient(test),
        audio_thread_context_(new AudioThreadContext(audio_manager)) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(
        base::Bind(&ServiceTestClient::Create, base::Unretained(this)));
  }

  ~ServiceTestClient() override { audio_thread_context_->QuitOnAudioThread(); }

 protected:
  bool OnServiceManagerConnectionLost() override { return true; }

  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void Create(service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

  // service_manager::mojom::ServiceFactory:
  void CreateService(service_manager::mojom::ServiceRequest request,
                     const std::string& name) override {
    if (name == mojom::kServiceName)
      audio_thread_context_->CreateServiceOnAudioThread(std::move(request));
  }

 private:
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
  scoped_refptr<AudioThreadContext> audio_thread_context_;
  DISALLOW_COPY_AND_ASSIGN(ServiceTestClient);
};

class AudioSystemToMojoAdapterIntegrationTest
    : public service_manager::test::ServiceTest {
 public:
  AudioSystemToMojoAdapterIntegrationTest() : ServiceTest("audio_unittests") {}

  ~AudioSystemToMojoAdapterIntegrationTest() override {}

 protected:
  // service_manager::test::ServiceTest:
  std::unique_ptr<service_manager::Service> CreateService() override {
    return base::MakeUnique<ServiceTestClient>(this, audio_manager_.get());
  }

  void SetUp() override {
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(true /*use_audio_thread_*/));
    ServiceTest::SetUp();
    audio_system_ = AudioSystemToMojoAdapter::Create(connector());
    tester_ = std::make_unique<media::AudioSystemTester>(audio_manager_.get(),
                                                         audio_system_.get());
  }

  void TearDown() override {
    tester_.reset();
    audio_system_.reset();

    // Deletes ServiceTestClient, which will result in posting
    // AuioThreadContext::QuitOnAudioThread() to AudioManager thread, so that
    // AudioService is delete there.
    ServiceTest::TearDown();

    // Joins AudioManager thread if it is used.
    audio_manager_->Shutdown();
  }

 protected:
  // bool use_audio_thread_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  std::unique_ptr<media::AudioSystemTester> tester_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapterIntegrationTest);
};

#define AUDIO_SYSTEM_CONFORMANCE_TEST(test_name)               \
  TEST_F(AudioSystemToMojoAdapterIntegrationTest, test_name) { \
    tester_->Test##test_name();                                \
  }

AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputStreamParametersNormal);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputStreamParametersNoDevice);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetOutputStreamParameters);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetDefaultOutputStreamParameters);
AUDIO_SYSTEM_CONFORMANCE_TEST(
    GetOutputStreamParametersForDefaultDeviceNoDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(
    GetOutputStreamParametersForNonDefaultDeviceNoDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasNoInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasOutputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasNoOutputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceDescriptions);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetOutputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetOutputDeviceDescriptions);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetAssociatedOutputDeviceID);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceInfoNoAssociation);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceInfoWithAssociation);

#undef AUDIO_SYSTEM_CONFORMANCE_TEST

// INSTANTIATE_TEST_CASE_P(, AudioSystemToMojoAdapterTest,
// testing::Values(false, true));

class AudioSystemToMojoAdapterTest : public testing::Test {
 public:
  AudioSystemToMojoAdapterTest() {}

  ~AudioSystemToMojoAdapterTest() override {}

 protected:
  void SetUp() override {
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(
            false /* use separate thread */));
    audio_system_ =
        std::make_unique<AudioSystemToMojoAdapter>(base::BindRepeating(
            &AudioSystemToMojoAdapterTest::BindAudioSystemInfoRequest,
            base::Unretained(this)));
    audio_system_tester_ = std::make_unique<media::AudioSystemTester>(
        audio_manager_.get(), audio_system_.get());
    audio_system_info_impl_ =
        std::make_unique<audio::AudioSystemInfo>(audio_manager_.get());
    audio_system_info_binding_ =
        std::make_unique<mojo::Binding<mojom::AudioSystemInfo>>(
            audio_system_info_impl_.get());
  }

  void TearDown() override {
    audio_system_tester_.reset();
    audio_system_.reset();
    audio_system_info_binding_->Close();
    audio_manager_->Shutdown();
  }

 protected:
  base::MessageLoop message_loop_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  std::unique_ptr<media::AudioSystemTester> audio_system_tester_;
  std::unique_ptr<mojom::AudioSystemInfo> audio_system_info_impl_;
  std::unique_ptr<mojo::Binding<mojom::AudioSystemInfo>>
      audio_system_info_binding_;

  bool fail_bind_request_ = false;
  MOCK_METHOD0(AudioSystemInfoRequested, void(void));

 private:
  void BindAudioSystemInfoRequest(mojom::AudioSystemInfoPtr* info_ptr) {
    AudioSystemInfoRequested();
    EXPECT_TRUE(audio_system_info_binding_)
        << "AudioSystemToMojoAdapter should not request AudioSysteInfo during "
           "construction";
    EXPECT_FALSE(audio_system_info_binding_->is_bound());
    if (fail_bind_request_)
      return;
    audio_system_info_binding_->Bind(mojo::MakeRequest(info_ptr));
  }

  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapterTest);
};

TEST_F(AudioSystemToMojoAdapterTest,
       GetInputStreamParametersNormalConnectionLost) {
  audio_manager_->SetHasInputDevices(true);
  audio_manager_->SetInputStreamParameters(
      media::AudioParameters::UnavailableDeviceParams());

  {
    base::RunLoop wait_loop;
    media::AudioSystemCallbackExpectations expectations;
    EXPECT_CALL(*this, AudioSystemInfoRequested()).Times(1);
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            media::AudioParameters::UnavailableDeviceParams()));
    wait_loop.Run();
  }

  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this, AudioSystemInfoRequested()).Times(0);
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        base::Bind(
            [](base::Closure quite_cb,
               const base::Optional<media::AudioParameters>& params) {
              EXPECT_FALSE(params);
              std::move(quite_cb).Run();
            },
            base::Passed(wait_loop.QuitClosure())));

    audio_system_info_binding_->Close();
    wait_loop.Run();
  }

  {
    fail_bind_request_ = true;
    base::RunLoop wait_loop;
    EXPECT_CALL(*this, AudioSystemInfoRequested()).Times(1);
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        base::Bind(
            [](base::Closure quite_cb,
               const base::Optional<media::AudioParameters>& params) {
              EXPECT_FALSE(params);
              std::move(quite_cb).Run();
            },
            base::Passed(wait_loop.QuitClosure())));
    wait_loop.Run();
    fail_bind_request_ = false;
  }

  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this, AudioSystemInfoRequested()).Times(1);
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        base::Bind(
            [](base::Closure quite_cb,
               const base::Optional<media::AudioParameters>& params) {
              EXPECT_TRUE(params->Equals(
                  media::AudioParameters::UnavailableDeviceParams()));
              std::move(quite_cb).Run();
            },
            base::Passed(wait_loop.QuitClosure())));
    wait_loop.Run();
  }
}

// TODO: do it in a subclass
#define AUDIO_SYSTEM_CONFORMANCE_TEST(test_name)             \
  TEST_F(AudioSystemToMojoAdapterTest, test_name) {          \
    EXPECT_CALL(*this, AudioSystemInfoRequested()).Times(1); \
    audio_system_tester_->Test##test_name();                 \
  }

AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputStreamParametersNormal);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputStreamParametersNoDevice);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetOutputStreamParameters);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetDefaultOutputStreamParameters);
AUDIO_SYSTEM_CONFORMANCE_TEST(
    GetOutputStreamParametersForDefaultDeviceNoDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(
    GetOutputStreamParametersForNonDefaultDeviceNoDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasNoInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasOutputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(HasNoOutputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceDescriptions);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetOutputDeviceDescriptionsNoInputDevices);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetOutputDeviceDescriptions);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetAssociatedOutputDeviceID);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceInfoNoAssociation);
AUDIO_SYSTEM_CONFORMANCE_TEST(GetInputDeviceInfoWithAssociation);

#undef AUDIO_SYSTEM_CONFORMANCE_TEST

}  // namespace audio
