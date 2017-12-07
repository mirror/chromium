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

class AudioSystemToMojoAdapterTest : public testing::Test {
 public:
  AudioSystemToMojoAdapterTest() {}

  ~AudioSystemToMojoAdapterTest() override {}

  void SetUp() override {
    audio_manager_ = std::make_unique<media::MockAudioManager>(
        std::make_unique<media::TestAudioThread>(
            false /* we do not use separate thread here */));
    audio_system_ =
        std::make_unique<AudioSystemToMojoAdapter>(base::BindRepeating(
            &AudioSystemToMojoAdapterTest::BindSystemInfoRequest,
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

  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapterTest);
};

TEST_F(AudioSystemToMojoAdapterTest,
       GetInputStreamParametersConnectionLossTest) {
  media::AudioSystemCallbackExpectations expectations;
  audio_manager_->SetHasInputDevices(true);
  audio_manager_->SetInputStreamParameters(
      media::AudioParameters::UnavailableDeviceParams());

  EXPECT_FALSE(system_info_binding_->is_bound());

  {  // GetInputStreamParameters succeeds.
    base::RunLoop wait_loop;
    // Should bind:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            media::AudioParameters::UnavailableDeviceParams()));
    wait_loop.Run();
    EXPECT_TRUE(system_info_binding_->is_bound());
  }

  EXPECT_TRUE(system_info_binding_->is_bound());

  {  // GetInputStreamParameters succeeds second time.
    base::RunLoop wait_loop;
    // Should be already bound:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            media::AudioParameters::UnavailableDeviceParams()));
    wait_loop.Run();
  }

  EXPECT_TRUE(system_info_binding_->is_bound());

  {  // GetInputStreamParameters fails correctly on connection loss.
    base::RunLoop wait_loop;
    // Should be already bound:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(0));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                            base::nullopt));
    system_info_binding_->Close();  // Connection loss.
    base::RunLoop().RunUntilIdle();
    wait_loop.Run();
  }

  EXPECT_FALSE(system_info_binding_->is_bound());

  {  // GetInputStreamParameters fails correctly on connection loss if already
     // unbound.
    base::RunLoop wait_loop;
    // Should re-bind after connection loss:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(FROM_HERE, wait_loop.QuitClosure(),
                                            base::nullopt));
    system_info_binding_->Close();  // Connection loss.
    base::RunLoop().RunUntilIdle();
    wait_loop.Run();
  }

  EXPECT_FALSE(system_info_binding_->is_bound());

  {  // GetInputStreamParameters finally succeeds again!
    base::RunLoop wait_loop;
    // Should bind:
    EXPECT_CALL(system_info_bind_requested_, Call()).Times(Exactly(1));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            media::AudioParameters::UnavailableDeviceParams()));
    wait_loop.Run();
  }

  EXPECT_TRUE(system_info_binding_->is_bound());
}

// TODO(olka): when reviewers are happy, add connection lost tests for each
// SystemInfo method. DO NOT COMMIT BEFORE THAT.

}  // namespace audio

// AudioSystem interface conformance tests.
// AudioSystemTestTemplate is defined in media, so should be its instantiations.
namespace media {

using AudioSystemToMojoAdapterTestVariations =
    testing::Types<audio::AudioSystemToMojoAdapterTest>;

INSTANTIATE_TYPED_TEST_CASE_P(AudioSystemToMojoAdapter,
                              AudioSystemTestTemplate,
                              AudioSystemToMojoAdapterTestVariations);
}  // namespace media
