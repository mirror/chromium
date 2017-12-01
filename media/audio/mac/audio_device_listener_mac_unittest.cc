// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/mac/audio_device_listener_mac.h"

#include <CoreAudio/AudioHardware.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "media/base/bind_to_current_loop.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class AudioDeviceListenerMacTest : public testing::Test {
 public:
  AudioDeviceListenerMacTest() {
    // It's important to create the device listener from the message loop in
    // order to ensure we don't end up with unbalanced TaskObserver calls.
    message_loop_.task_runner()->PostTask(
        FROM_HERE, base::Bind(&AudioDeviceListenerMacTest::CreateDeviceListener,
                              base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  virtual ~AudioDeviceListenerMacTest() {
    // It's important to destroy the device listener from the message loop in
    // order to ensure we don't end up with unbalanced TaskObserver calls.
    message_loop_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&AudioDeviceListenerMacTest::DestroyDeviceListener,
                   base::Unretained(this)));
    base::RunLoop().RunUntilIdle();
  }

  void CreateDeviceListener() {
    // Force a post task using BindToCurrentLoop() to ensure device listener
    // internals are working correctly.
    device_listener_.reset(new AudioDeviceListenerMac(BindToCurrentLoop(
        base::Bind(&AudioDeviceListenerMacTest::OnDeviceChange,
                   base::Unretained(this)))));
  }

  void DestroyDeviceListener() { device_listener_.reset(); }

  bool ListenerIsValid() { return !device_listener_->listener_cb_.is_null(); }

  // Simulate a device change where no output devices are available.
  bool SimulateDefaultOutputDeviceChange() {
    // Include multiple addresses to ensure only a single device change event
    // occurs.
    const AudioObjectPropertyAddress addresses[] = {
        AudioDeviceListenerMac::kDefaultOutputDeviceChangePropertyAddress,
        {kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
         kAudioObjectPropertyElementMaster}};

    return noErr ==
           device_listener_->OnDefaultOutputDeviceChanged(
               kAudioObjectSystemObject, 1, addresses, device_listener_.get());
  }

  bool SimulateDefaultInputDeviceChange() {
    // Include multiple addresses to ensure only a single device change event
    // occurs.
    const AudioObjectPropertyAddress addresses[] = {
        AudioDeviceListenerMac::kDefaultInputDeviceChangePropertyAddress,
        {kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
         kAudioObjectPropertyElementMaster}};

    return noErr ==
           device_listener_->OnDefaultInputDeviceChanged(
               kAudioObjectSystemObject, 1, addresses, device_listener_.get());
  }

  bool SimulateDeviceAdditionRemoval() {
    const AudioObjectPropertyAddress addresses[] = {
        AudioDeviceListenerMac::kDevicesPropertyAddress,
        {kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
         kAudioObjectPropertyElementMaster}};

    return noErr ==
           device_listener_->OnDevicesAddedOrRemoved(
               kAudioObjectSystemObject, 1, addresses, device_listener_.get());
  }

  MOCK_METHOD0(OnDeviceChange, void());

 protected:
  base::MessageLoop message_loop_;
  std::unique_ptr<AudioDeviceListenerMac> device_listener_;

  DISALLOW_COPY_AND_ASSIGN(AudioDeviceListenerMacTest);
};

// Simulate a device change event and ensure we get the right callback.
TEST_F(AudioDeviceListenerMacTest, DefaultOutputDeviceChange) {
  ASSERT_TRUE(ListenerIsValid());
  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateDefaultOutputDeviceChange());
  base::RunLoop().RunUntilIdle();
}

TEST_F(AudioDeviceListenerMacTest, DefaultInputDeviceChange) {
  ASSERT_TRUE(ListenerIsValid());
  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateDefaultInputDeviceChange());
  base::RunLoop().RunUntilIdle();
}

TEST_F(AudioDeviceListenerMacTest, DevicesAddedOrRemoved) {
  ASSERT_TRUE(ListenerIsValid());
  EXPECT_CALL(*this, OnDeviceChange()).Times(1);
  ASSERT_TRUE(SimulateDeviceAdditionRemoval());
  base::RunLoop().RunUntilIdle();
}

}  // namespace media
