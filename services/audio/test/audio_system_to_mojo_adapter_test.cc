// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/audio_system_to_mojo_adapter.h"
#include "media/audio/audio_system_test_util.h"
#include "media/audio/test_audio_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/audio/audio_system_info.h"
#include "services/audio/in_process_audio_manager_accessor.h"

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
            &AudioSystemToMojoAdapterTest::BindAudioSystemInfoRequest,
            base::Unretained(this)));
    audio_system_info_impl_ =
        std::make_unique<audio::AudioSystemInfo>(audio_manager_.get());
    audio_system_info_binding_ =
        std::make_unique<mojo::Binding<mojom::AudioSystemInfo>>(
            audio_system_info_impl_.get());
  }

  void TearDown() override {
    audio_system_.reset();
    audio_system_info_binding_->Close();
    audio_manager_->Shutdown();
  }

 protected:
  media::MockAudioManager* audio_manager() { return audio_manager_.get(); }
  media::AudioSystem* audio_system() { return audio_system_.get(); }

  NiceMock<MockFunction<void(bool success)>> audio_system_info_bind_success_;
  bool fail_bind_request_ = false;

  base::MessageLoop message_loop_;

  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  std::unique_ptr<mojom::AudioSystemInfo> audio_system_info_impl_;
  std::unique_ptr<mojo::Binding<mojom::AudioSystemInfo>>
      audio_system_info_binding_;

 private:
  void BindAudioSystemInfoRequest(mojom::AudioSystemInfoPtr* info_ptr) {
    EXPECT_TRUE(audio_system_info_binding_)
        << "AudioSystemToMojoAdapter should not request AudioSysteInfo during "
           "construction";
    EXPECT_FALSE(audio_system_info_binding_->is_bound());
    audio_system_info_bind_success_.Call(!fail_bind_request_);
    if (fail_bind_request_)
      return;
    audio_system_info_binding_->Bind(mojo::MakeRequest(info_ptr));
  }

  DISALLOW_COPY_AND_ASSIGN(AudioSystemToMojoAdapterTest);
};

TEST_F(AudioSystemToMojoAdapterTest, GetInputStreamParametersConnectionTest) {
  media::AudioSystemCallbackExpectations expectations;
  audio_manager_->SetHasInputDevices(true);
  audio_manager_->SetInputStreamParameters(
      media::AudioParameters::UnavailableDeviceParams());

  EXPECT_FALSE(audio_system_info_binding_->is_bound());

  {  // GetInputStreamParameters succeeds.
    base::RunLoop wait_loop;
    // Should bind:
    EXPECT_CALL(audio_system_info_bind_success_, Call(true)).Times(Exactly(1));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            media::AudioParameters::UnavailableDeviceParams()));
    wait_loop.Run();
    EXPECT_TRUE(audio_system_info_binding_->is_bound());
  }

  EXPECT_TRUE(audio_system_info_binding_->is_bound());

  {  // GetInputStreamParameters succeeds second time.
    base::RunLoop wait_loop;
    // Should be already bound:
    EXPECT_CALL(audio_system_info_bind_success_, Call(_)).Times(Exactly(0));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            media::AudioParameters::UnavailableDeviceParams()));
    wait_loop.Run();
  }

  EXPECT_TRUE(audio_system_info_binding_->is_bound());

  {  // GetInputStreamParameters fails correctly on connection loss.
    base::RunLoop wait_loop;
    // Should be already bound:
    EXPECT_CALL(audio_system_info_bind_success_, Call(_)).Times(Exactly(0));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            base::Optional<media::AudioParameters>()));
    audio_system_info_binding_->Close();  // Connection loss.
    wait_loop.Run();
  }

  EXPECT_FALSE(audio_system_info_binding_->is_bound());

  {  // GetInputStreamParameters fails correctly on connection loss if already
     // unbound.
    base::RunLoop wait_loop;
    // Should re-bind after connection loss:
    EXPECT_CALL(audio_system_info_bind_success_, Call(true)).Times(Exactly(1));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            base::Optional<media::AudioParameters>()));
    audio_system_info_binding_->Close();  // Connection loss.
    wait_loop.Run();
    fail_bind_request_ = false;
  }

  EXPECT_FALSE(audio_system_info_binding_->is_bound());

  {  // GetInputStreamParameters fails correctly on bind request failure.
    fail_bind_request_ = true;  // Bind request will fail.
    base::RunLoop wait_loop;
    // Should re-bind after connection loss (unsuccessfully though):
    EXPECT_CALL(audio_system_info_bind_success_, Call(false)).Times(Exactly(1));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            base::Optional<media::AudioParameters>()));
    wait_loop.Run();
    fail_bind_request_ = false;
  }

  EXPECT_FALSE(audio_system_info_binding_->is_bound());

  {  // GetInputStreamParameters finally succeeds again!
    base::RunLoop wait_loop;
    // Should bind:
    EXPECT_CALL(audio_system_info_bind_success_, Call(true)).Times(Exactly(1));
    audio_system_->GetInputStreamParameters(
        media::AudioDeviceDescription::kDefaultDeviceId,
        expectations.GetAudioParamsCallback(
            FROM_HERE, wait_loop.QuitClosure(),
            media::AudioParameters::UnavailableDeviceParams()));
    wait_loop.Run();
  }

  EXPECT_TRUE(audio_system_info_binding_->is_bound());
}

// TODO: add connection lost and binding fail tests for each AudioSystemInfo
// method.

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
