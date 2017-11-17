// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system_test_util.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "services/audio/audio_service.h"
#include "services/audio/audio_system_info.h"
#include "services/audio/in_process_audio_manager_accessor.h"
#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/interfaces/service_factory.mojom.h"

using testing::Exactly;
using testing::Invoke;

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
                  base::TimeDelta::FromMilliseconds(100)),
              std::move(request));
      audio_service_context_->SetQuitClosure(base::Bind(
          &AudioThreadContext::QuitOnAudioThread, base::Unretained(this)));
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
    virtual ~AudioThreadContext() {
      if (audio_service_context_)
        audio_service_context_->QuitNow();
    };

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

  ~ServiceTestClient() override {}

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

template <bool use_audio_thread>
class InProcessAudioServiceTest : public service_manager::test::ServiceTest {
 public:
  InProcessAudioServiceTest() : ServiceTest("audio_unittests") {}

  ~InProcessAudioServiceTest() override {}

 protected:
  // service_manager::test::ServiceTest:
  std::unique_ptr<service_manager::Service> CreateService() override {
    return base::MakeUnique<ServiceTestClient>(this, audio_manager_.get());
  }

  void SetUp() override {
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(use_audio_thread));
    ServiceTest::SetUp();
    audio_system_ = AudioSystemToMojoAdapter::Create(connector());
  }

  void TearDown() override {
    audio_system_.reset();

    // Deletes ServiceTestClient, which will result in posting
    // AuioThreadContext::QuitOnAudioThread() to AudioManager thread, so that
    // AudioService is delete there.
    ServiceTest::TearDown();

    // Joins AudioManager thread if it is used.
    audio_manager_->Shutdown();
  }

 protected:
  media::MockAudioManager* audio_manager() { return audio_manager_.get(); }
  media::AudioSystem* audio_system() { return audio_system_.get(); }

  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InProcessAudioServiceTest);
};

class ServiceObserverMock
    : public service_manager::mojom::ServiceManagerListener {
 public:
  ServiceObserverMock(
      const std::string& service_name,
      service_manager::mojom::ServiceManagerListenerRequest request)
      : service_name_(service_name), binding_(this, std::move(request)) {}
  ~ServiceObserverMock() override{};

  MOCK_METHOD0(Initialized, void(void));
  MOCK_METHOD0(ServiceStarted, void(void));
  MOCK_METHOD0(ServiceStopped, void(void));

  // mojom::ServiceManagerListener implementation.
  void OnInit(std::vector<service_manager::mojom::RunningServiceInfoPtr>
                  instances) override {
    Initialized();
  }

  void OnServiceCreated(
      service_manager::mojom::RunningServiceInfoPtr instance) override {}

  void OnServiceStarted(const service_manager::Identity& identity,
                        uint32_t pid) override {
    if (identity.name() == service_name_)
      ServiceStarted();
  }

  void OnServiceFailedToStart(
      const service_manager::Identity& identity) override {}

  void OnServicePIDReceived(const service_manager::Identity& identity,
                            uint32_t pid) override {}

  void OnServiceStopped(const service_manager::Identity& identity) {
    if (identity.name() == service_name_)
      ServiceStopped();
  }

 private:
  const std::string service_name_;
  mojo::Binding<service_manager::mojom::ServiceManagerListener> binding_;
};

template <class TestBase>
class LifetimeAudioServiceTestTemplate : public TestBase {
 public:
  LifetimeAudioServiceTestTemplate() {}

  ~LifetimeAudioServiceTestTemplate() override {}

 protected:
  void SetUp() override {
    TestBase::SetUp();

    service_manager::mojom::ServiceManagerPtr service_manager;
    TestBase::connector()->BindInterface(service_manager::mojom::kServiceName,
                                         &service_manager);
    service_manager::mojom::ServiceManagerListenerPtr listener;
    service_observer_ = std::make_unique<ServiceObserverMock>(
        mojom::kServiceName, mojo::MakeRequest(&listener));

    base::RunLoop wait_loop;
    EXPECT_CALL(*service_observer_, Initialized())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    service_manager->AddListener(std::move(listener));
    wait_loop.Run();
  }

 protected:
  std::unique_ptr<ServiceObserverMock> service_observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LifetimeAudioServiceTestTemplate);
};

TYPED_TEST_CASE_P(LifetimeAudioServiceTestTemplate);

TYPED_TEST_P(LifetimeAudioServiceTestTemplate,
             ServiceQuitsWhenClientDisconnects) {
  mojom::AudioSystemInfoPtr info;
  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStarted())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    this->connector()->BindInterface(mojom::kServiceName, &info);
    wait_loop.Run();
  }

  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStopped())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    info.reset();
    wait_loop.Run();
  }
}

TYPED_TEST_P(LifetimeAudioServiceTestTemplate,
             ServiceQuitsWhenLastClientDisconnects) {
  mojom::AudioSystemInfoPtr info;
  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStarted())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    this->connector()->BindInterface(mojom::kServiceName, &info);
    wait_loop.Run();
  }

  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStopped())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    EXPECT_CALL(*this->service_observer_, ServiceStarted()).Times(Exactly(0));

    mojom::AudioSystemInfoPtr info2;
    this->connector()->BindInterface(mojom::kServiceName, &info2);
    mojom::AudioSystemInfoPtr info3;
    this->connector()->BindInterface(mojom::kServiceName, &info3);

    info.reset();
    info2.reset();
    info3.reset();
    wait_loop.Run();
  }
}

TYPED_TEST_P(LifetimeAudioServiceTestTemplate,
             ServiceRestartsWhenClientReconnects) {
  mojom::AudioSystemInfoPtr info;
  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStarted())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    this->connector()->BindInterface(mojom::kServiceName, &info);
    wait_loop.Run();
  }

  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStopped())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    info.reset();
    wait_loop.Run();
  }

  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStarted())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    this->connector()->BindInterface(mojom::kServiceName, &info);
    wait_loop.Run();
  }

  {
    base::RunLoop wait_loop;
    EXPECT_CALL(*this->service_observer_, ServiceStopped())
        .WillOnce(Invoke(&wait_loop, &base::RunLoop::Quit));
    info.reset();
    wait_loop.Run();
  }
}

REGISTER_TYPED_TEST_CASE_P(LifetimeAudioServiceTestTemplate,
                           ServiceQuitsWhenClientDisconnects,
                           ServiceQuitsWhenLastClientDisconnects,
                           ServiceRestartsWhenClientReconnects);

using AudioServiceLifetimeTestVariations =
    testing::Types<InProcessAudioServiceTest<false>,
                   InProcessAudioServiceTest<true>>;

INSTANTIATE_TYPED_TEST_CASE_P(AudioServiceLifetime,
                              LifetimeAudioServiceTestTemplate,
                              AudioServiceLifetimeTestVariations);

}  // namespace audio

// AudioSystem interface conformance tests.
// AudioSystemTestTemplate is defined in media, so should be its instantiations.
namespace media {

using InProcessAudioServiceTestVariations =
    testing::Types<audio::InProcessAudioServiceTest<false>,
                   audio::InProcessAudioServiceTest<true>>;

INSTANTIATE_TYPED_TEST_CASE_P(InProcessAudioService,
                              AudioSystemTestTemplate,
                              InProcessAudioServiceTestVariations);
}  // namespace media
