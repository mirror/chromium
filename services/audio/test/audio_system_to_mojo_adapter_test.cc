// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"
#include "media/audio/audio_system_test_util.h"
#include "media/audio/test_audio_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/audio/in_process_audio_manager_accessor.h"
#include "services/audio/system_info.h"

using testing::_;
using testing::Exactly;
using testing::MockFunction;
using testing::NiceMock;

namespace audio {

// Base fixture for all the tests below.
class AudioSystemToMojoAdapterTestBase : public testing::Test {
 public:
  AudioSystemToMojoAdapterTestBase() {}

  ~AudioSystemToMojoAdapterTestBase() override {}

  void SetUp() override {
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(
            false /* we do not use separate thread here */));
    audio_system_ =
        std::make_unique<AudioSystemToMojoAdapter>(base::BindRepeating(
            &AudioSystemToMojoAdapterTestBase::BindSystemInfoRequest,
            base::Unretained(this)));
    system_info_impl_ =
        std::make_unique<audio::SystemInfo>(audio_manager_.get());
    system_info_binding_ = std::make_unique<mojo::Binding<mojom::SystemInfo>>(
        system_info_impl_.get());
  }

  void TearDown() override {
    audio_system_.reset();
    system_info_binding_->Close();
    audio_manager_->Shutdown();
  }

 protected:
  // Required by AudioSystemTestTemplate:
  media::MockAudioManager* audio_manager() { return audio_manager_.get(); }
  media::AudioSystem* audio_system() { return audio_system_.get(); }

  // Called when SystemInfo bind request is received. Nice mock because
  // AudioSystem conformance tests won't set expecnations.
  NiceMock<MockFunction<void(void)>> system_info_bind_requested_;

  base::MessageLoop message_loop_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  std::unique_ptr<mojom::SystemInfo> system_info_impl_;
  std::unique_ptr<mojo::Binding<mojom::SystemInfo>> system_info_binding_;

 private:
  void BindSystemInfoRequest(mojom::SystemInfoRequest info_request) {
    EXPECT_TRUE(system_info_binding_)
        << "AudioSystemToMojoAdapter should not request AudioSysteInfo during "
           "construction";
    EXPECT_FALSE(system_info_binding_->is_bound());
    system_info_bind_requested_.Call();
    system_info_binding_->Bind(std::move(info_request));
  }

  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapterTestBase);
};

// Base fixture for connection loss tests.
class AudioSystemToMojoAdapterConnectionLossTest
    : public AudioSystemToMojoAdapterTestBase {
 public:
  AudioSystemToMojoAdapterConnectionLossTest() {}

  ~AudioSystemToMojoAdapterConnectionLossTest() override {}

  void SetUp() override {
    AudioSystemToMojoAdapterTestBase::SetUp();
    params_ = media::AudioParameters::UnavailableDeviceParams();
    audio_manager()->SetInputStreamParameters(params_);
    audio_manager()->SetOutputStreamParameters(params_);
    audio_manager()->SetDefaultOutputStreamParameters(params_);

    auto get_device_descriptions =
        [](const media::AudioDeviceDescriptions* source,
           media::AudioDeviceDescriptions* destination) {
          destination->insert(destination->end(), source->begin(),
                              source->end());
        };
    device_descriptions_.emplace_back("device_name", "device_id", "group_id");
    audio_manager()->SetInputDeviceDescriptionsCallback(base::BindRepeating(
        get_device_descriptions, base::Unretained(&device_descriptions_)));
    audio_manager()->SetOutputDeviceDescriptionsCallback(base::BindRepeating(
        get_device_descriptions, base::Unretained(&device_descriptions_)));

    associated_id_ = "associated_id";
    audio_manager()->SetAssociatedOutputDeviceIDCallback(base::BindRepeating(
        [](const std::string* result, const std::string&) -> std::string {
          return *result;
        },
        base::Unretained(&associated_id_)));
  }

 protected:
  void GetDeviceDescriptionsTest(bool for_input) {
    EXPECT_FALSE(system_info_binding_->is_bound());
    {  // Succeeds.
      base::RunLoop wait_loop;
      // Should bind:
      EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
      audio_system_->GetDeviceDescriptions(
          for_input,
          expectations_.GetDeviceDescriptionsCallback(
              FROM_HERE, wait_loop.QuitClosure(), device_descriptions_));
      wait_loop.Run();
    }
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(system_info_binding_->is_bound());
    {  // Fails correctly on connection loss.
      base::RunLoop wait_loop;
      // Should be already bound:
      EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
      audio_system_->GetDeviceDescriptions(
          for_input, expectations_.GetDeviceDescriptionsCallback(
                         FROM_HERE, wait_loop.QuitClosure(),
                         media::AudioDeviceDescriptions()));
      system_info_binding_->Close();  // Connection loss.
      base::RunLoop().RunUntilIdle();
      wait_loop.Run();
    }
    EXPECT_FALSE(system_info_binding_->is_bound());
  }

  void HasDevicesTest(bool for_input) {
    auto has_devices = for_input ? &media::AudioSystem::HasInputDevices
                                 : &media::AudioSystem::HasOutputDevices;
    EXPECT_FALSE(system_info_binding_->is_bound());
    {  // Succeeds.
      base::RunLoop wait_loop;
      // Should bind:
      EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
      (audio_system()->*has_devices)(expectations_.GetBoolCallback(
          FROM_HERE, wait_loop.QuitClosure(), true));
      wait_loop.Run();
    }
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(system_info_binding_->is_bound());
    {  // Fails correctly on connection loss.
      base::RunLoop wait_loop;
      // Should be already bound:
      EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
      (audio_system()->*has_devices)(expectations_.GetBoolCallback(
          FROM_HERE, wait_loop.QuitClosure(), false));
      system_info_binding_->Close();  // Connection loss.
      base::RunLoop().RunUntilIdle();
      wait_loop.Run();
    }
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(system_info_binding_->is_bound());
  }

  void GetStreamParametersTest(bool for_input) {
    auto get_stream_parameters =
        for_input ? &media::AudioSystem::GetInputStreamParameters
                  : &media::AudioSystem::GetOutputStreamParameters;
    EXPECT_FALSE(system_info_binding_->is_bound());
    {  // Succeeds.
      base::RunLoop wait_loop;
      // Should bind:
      EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
      (audio_system()->*get_stream_parameters)(
          media::AudioDeviceDescription::kDefaultDeviceId,
          expectations_.GetAudioParamsCallback(
              FROM_HERE, wait_loop.QuitClosure(), params_));
      wait_loop.Run();
    }
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(system_info_binding_->is_bound());
    {  // Fails correctly on connection loss.
      base::RunLoop wait_loop;
      // Should be already bound:
      EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
      (audio_system()->*get_stream_parameters)(
          media::AudioDeviceDescription::kDefaultDeviceId,
          expectations_.GetAudioParamsCallback(
              FROM_HERE, wait_loop.QuitClosure(), base::nullopt));
      system_info_binding_->Close();  // Connection loss.
      base::RunLoop().RunUntilIdle();
      wait_loop.Run();
    }
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(system_info_binding_->is_bound());
  }

  std::string associated_id_;
  media::AudioParameters params_;
  media::AudioDeviceDescriptions device_descriptions_;
  media::AudioSystemCallbackExpectations expectations_;

  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapterConnectionLossTest);
};

// This test covers various scenarios of connection loss/restore, and the
// next tests only verify that we receive a correct callback if the connection
// is lost.
TEST_F(AudioSystemToMojoAdapterConnectionLossTest,
       SetAssociatedOutputDeviceIDFullConnectionTest) {
  EXPECT_FALSE(system_info_binding_->is_bound());
  {  // Succeeds.
    base::RunLoop wait_loop;
    // Should bind:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
    audio_system_->GetAssociatedOutputDeviceID(
        std::string(), expectations_.GetDeviceIdCallback(
                           FROM_HERE, wait_loop.QuitClosure(), associated_id_));
    wait_loop.Run();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(system_info_binding_->is_bound());
  {  // Succeeds second time.
    base::RunLoop wait_loop;
    // Should be already bound:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
    audio_system_->GetAssociatedOutputDeviceID(
        std::string(), expectations_.GetDeviceIdCallback(
                           FROM_HERE, wait_loop.QuitClosure(), associated_id_));
    wait_loop.Run();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(system_info_binding_->is_bound());
  {  // Fails correctly on connection loss.
    base::RunLoop wait_loop;
    // Should be already bound:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
    audio_system_->GetAssociatedOutputDeviceID(
        std::string(), expectations_.GetDeviceIdCallback(
                           FROM_HERE, wait_loop.QuitClosure(), std::string()));
    system_info_binding_->Close();  // Connection loss.
    wait_loop.Run();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(system_info_binding_->is_bound());
  {  // Fails correctly on connection loss if already unbound.
    base::RunLoop wait_loop;
    // Should re-bind after connection loss:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
    audio_system_->GetAssociatedOutputDeviceID(
        std::string(), expectations_.GetDeviceIdCallback(
                           FROM_HERE, wait_loop.QuitClosure(), std::string()));
    system_info_binding_->Close();  // Connection loss.
    wait_loop.Run();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(system_info_binding_->is_bound());
  {  // Finally succeeds again!
    base::RunLoop wait_loop;
    // Should bind:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
    audio_system_->GetAssociatedOutputDeviceID(
        std::string(), expectations_.GetDeviceIdCallback(
                           FROM_HERE, wait_loop.QuitClosure(), associated_id_));
    wait_loop.Run();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(system_info_binding_->is_bound());
}

TEST_F(AudioSystemToMojoAdapterConnectionLossTest, GetInputStreamParameters) {
  GetStreamParametersTest(true);
}

TEST_F(AudioSystemToMojoAdapterConnectionLossTest, GetOutputStreamParameters) {
  GetStreamParametersTest(false);
}

TEST_F(AudioSystemToMojoAdapterConnectionLossTest, HasInputDevices) {
  HasDevicesTest(true);
}

TEST_F(AudioSystemToMojoAdapterConnectionLossTest, HasOutputDevices) {
  HasDevicesTest(false);
}

TEST_F(AudioSystemToMojoAdapterConnectionLossTest, GetInputDeviceDescriptions) {
  GetDeviceDescriptionsTest(true);
}

TEST_F(AudioSystemToMojoAdapterConnectionLossTest,
       GetOutputDeviceDescriptions) {
  GetDeviceDescriptionsTest(false);
}

TEST_F(AudioSystemToMojoAdapterConnectionLossTest, GetInputDeviceInfo) {
  EXPECT_FALSE(system_info_binding_->is_bound());
  {  // Succeeds.
    base::RunLoop wait_loop;
    // Should bind:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
    audio_system_->GetInputDeviceInfo(
        "device-id", expectations_.GetInputDeviceInfoCallback(
                         FROM_HERE, wait_loop.QuitClosure(), params_, params_,
                         associated_id_));
    wait_loop.Run();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(system_info_binding_->is_bound());
  {  // Fails correctly on connection loss.
    base::RunLoop wait_loop;
    // Should be already bound:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
    audio_system_->GetInputDeviceInfo(
        "device-id", expectations_.GetInputDeviceInfoCallback(
                         FROM_HERE, wait_loop.QuitClosure(), base::nullopt,
                         base::nullopt, std::string()));
    system_info_binding_->Close();  // Connection loss.
    wait_loop.Run();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(system_info_binding_->is_bound());
}

}  // namespace audio

// AudioSystem interface conformance tests.
// AudioSystemTestTemplate is defined in media, so should be its instantiations.
namespace media {

using AudioSystemToMojoAdapterTestVariations =
    testing::Types<audio::AudioSystemToMojoAdapterTestBase>;

INSTANTIATE_TYPED_TEST_CASE_P(AudioSystemToMojoAdapter,
                              AudioSystemTestTemplate,
                              AudioSystemToMojoAdapterTestVariations);
}  // namespace media
