// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/mac/audio_device_listener_mac.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/mac/mac_logging.h"
#include "base/message_loop/message_loop.h"
#include "base/pending_task.h"

namespace media {

// Property address to monitor for device changes.
const AudioObjectPropertyAddress
    AudioDeviceListenerMac::kDefaultOutputDeviceChangePropertyAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster};

const AudioObjectPropertyAddress
    AudioDeviceListenerMac::kDefaultInputDeviceChangePropertyAddress = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster};

const AudioObjectPropertyAddress
    AudioDeviceListenerMac::kDevicesPropertyAddress = {
        kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster};

// Callback from the system when the default device changes; this must be called
// on the MessageLoop that created the AudioManager.
// static
OSStatus AudioDeviceListenerMac::OnDefaultOutputDeviceChanged(
    AudioObjectID object,
    UInt32 num_addresses,
    const AudioObjectPropertyAddress addresses[],
    void* context) {
  if (object != kAudioObjectSystemObject)
    return noErr;

  for (UInt32 i = 0; i < num_addresses; ++i) {
    if (addresses[i].mSelector ==
            kDefaultOutputDeviceChangePropertyAddress.mSelector &&
        addresses[i].mScope ==
            kDefaultOutputDeviceChangePropertyAddress.mScope &&
        addresses[i].mElement ==
            kDefaultOutputDeviceChangePropertyAddress.mElement &&
        context) {
      static_cast<AudioDeviceListenerMac*>(context)->listener_cb_.Run();
      break;
    }
  }

  return noErr;
}

OSStatus AudioDeviceListenerMac::OnDefaultInputDeviceChanged(
    AudioObjectID object,
    UInt32 num_addresses,
    const AudioObjectPropertyAddress addresses[],
    void* context) {
  if (object != kAudioObjectSystemObject)
    return noErr;

  for (UInt32 i = 0; i < num_addresses; ++i) {
    if (addresses[i].mSelector ==
            kDefaultInputDeviceChangePropertyAddress.mSelector &&
        addresses[i].mScope ==
            kDefaultInputDeviceChangePropertyAddress.mScope &&
        addresses[i].mElement ==
            kDefaultInputDeviceChangePropertyAddress.mElement &&
        context) {
      static_cast<AudioDeviceListenerMac*>(context)->listener_cb_.Run();
      break;
    }
  }

  return noErr;
}

OSStatus AudioDeviceListenerMac::OnDevicesAddedOrRemoved(
    AudioObjectID object,
    UInt32 num_addresses,
    const AudioObjectPropertyAddress addresses[],
    void* context) {
  if (object != kAudioObjectSystemObject)
    return noErr;

  for (UInt32 i = 0; i < num_addresses; ++i) {
    if (addresses[i].mSelector == kDevicesPropertyAddress.mSelector &&
        addresses[i].mScope == kDevicesPropertyAddress.mScope &&
        addresses[i].mElement == kDevicesPropertyAddress.mElement && context) {
      static_cast<AudioDeviceListenerMac*>(context)->listener_cb_.Run();
      break;
    }
  }

  return noErr;
}

AudioDeviceListenerMac::AudioDeviceListenerMac(const base::Closure& listener_cb,
                                               bool monitor_default_output,
                                               bool monitor_default_input,
                                               bool monitor_addition_removal)
    : monitor_default_output_(monitor_default_output),
      monitor_default_input_(monitor_default_input),
      monitor_addition_removal_(monitor_addition_removal) {
  if (monitor_default_output_) {
    OSStatus result = AudioObjectAddPropertyListener(
        kAudioObjectSystemObject, &kDefaultOutputDeviceChangePropertyAddress,
        &AudioDeviceListenerMac::OnDefaultOutputDeviceChanged, this);
    if (result != noErr) {
      OSSTATUS_DLOG(ERROR, result)
          << "AudioObjectAddPropertyListener() failed!";
      return;
    }
  }

  if (monitor_default_input_) {
    OSStatus result = AudioObjectAddPropertyListener(
        kAudioObjectSystemObject, &kDefaultInputDeviceChangePropertyAddress,
        &AudioDeviceListenerMac::OnDefaultInputDeviceChanged, this);
    if (result != noErr) {
      OSSTATUS_DLOG(ERROR, result)
          << "AudioObjectAddPropertyListener() failed!";
      return;
    }
  }

  if (monitor_addition_removal) {
    OSStatus result = AudioObjectAddPropertyListener(
        kAudioObjectSystemObject, &kDevicesPropertyAddress,
        &AudioDeviceListenerMac::OnDevicesAddedOrRemoved, this);
    if (result != noErr) {
      OSSTATUS_DLOG(ERROR, result)
          << "AudioObjectAddPropertyListener() failed!";
      return;
    }
  }

  listener_cb_ = listener_cb;
}

AudioDeviceListenerMac::~AudioDeviceListenerMac() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (listener_cb_.is_null())
    return;

  // Since we're running on the same CFRunLoop, there can be no outstanding
  // callbacks in flight.
  if (monitor_default_output_) {
    OSStatus result = AudioObjectRemovePropertyListener(
        kAudioObjectSystemObject, &kDefaultOutputDeviceChangePropertyAddress,
        &AudioDeviceListenerMac::OnDefaultOutputDeviceChanged, this);
    OSSTATUS_DLOG_IF(ERROR, result != noErr, result)
        << "AudioObjectRemovePropertyListener() failed!";
  }

  if (monitor_default_input_) {
    OSStatus result = AudioObjectRemovePropertyListener(
        kAudioObjectSystemObject, &kDefaultInputDeviceChangePropertyAddress,
        &AudioDeviceListenerMac::OnDefaultInputDeviceChanged, this);
    OSSTATUS_DLOG_IF(ERROR, result != noErr, result)
        << "AudioObjectRemovePropertyListener() failed!";
  }

  if (monitor_addition_removal_) {
    OSStatus result = AudioObjectRemovePropertyListener(
        kAudioObjectSystemObject, &kDevicesPropertyAddress,
        &AudioDeviceListenerMac::OnDevicesAddedOrRemoved, this);
    OSSTATUS_DLOG_IF(ERROR, result != noErr, result)
        << "AudioObjectRemovePropertyListener() failed!";
  }
}

}  // namespace media
