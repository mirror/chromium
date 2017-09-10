// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/mac/scoped_audio_unit.h"

#include "base/mac/mac_logging.h"

namespace media {

static void DestroyAudioUnit(AudioUnit audio_unit,
                             AUElement element,
                             bool enable_io) {
  if (!audio_unit)
    return;

  if (enable_io) {
    UInt32 enable = 1;
    OSStatus result = AudioUnitSetProperty(
        audio_unit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output,
        AUElement::OUTPUT, &enable, sizeof(enable));
    OSSTATUS_DLOG_IF(ERROR, result != noErr, result)
        << "kAudioOutputUnitProperty_EnableIO restore failed";
  }

  OSStatus result = AudioUnitUninitialize(audio_unit);
  OSSTATUS_DLOG_IF(ERROR, result != noErr, result)
      << "AudioUnitUninitialize() failed : " << audio_unit;
  result = AudioComponentInstanceDispose(audio_unit);
  OSSTATUS_DLOG_IF(ERROR, result != noErr, result)
      << "AudioComponentInstanceDispose() failed : " << audio_unit;
}

ScopedAudioUnit::ScopedAudioUnit(AudioDeviceID device,
                                 AUElement element,
                                 bool apply_voice_processing)
    : element_(element) {
  LOG(ERROR) << __FUNCTION__;
  AudioComponentDescription desc = {kAudioUnitType_Output,
                                    apply_voice_processing
                                        ? kAudioUnitSubType_VoiceProcessingIO
                                        : kAudioUnitSubType_HALOutput,
                                    kAudioUnitManufacturer_Apple, 0, 0};
  AudioComponent comp = AudioComponentFindNext(0, &desc);
  if (!comp)
    return;

  AudioUnit audio_unit;
  OSStatus result = AudioComponentInstanceNew(comp, &audio_unit);
  if (result != noErr) {
    OSSTATUS_DLOG(ERROR, result) << "AudioComponentInstanceNew() failed.";
    return;
  }

  result = AudioUnitSetProperty(
      audio_unit, kAudioOutputUnitProperty_CurrentDevice,
      kAudioUnitScope_Global, element, &device, sizeof(AudioDeviceID));
  // kAudioUnitErr_InvalidPropertyValue might be returned if output has been
  // enabled on the input unit, which for example happens implicitly when we
  // enable voice processing.
  if (result == kAudioUnitErr_InvalidPropertyValue &&
      element == AUElement::INPUT) {
    /*UInt32 enable = 0;
    result = AudioUnitSetProperty(
        audio_unit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output,
        AUElement::OUTPUT, &enable, sizeof(enable));
    // TODO(tommi): This^^^ will affect any current streams using this device!
    // We need to restore this if we change it and likely figure out a way to
    // avoid running into this situation. (never have more than one input
    // stream active?)
    if (result == noErr) {
      reenable_io_ = true;
      // Try again.
      result = AudioUnitSetProperty(
          audio_unit, kAudioOutputUnitProperty_CurrentDevice,
          kAudioUnitScope_Global, element, &device, sizeof(AudioDeviceID));
    }*/
  }

  if (result == noErr) {
    audio_unit_ = audio_unit;
    return;
  }

  OSSTATUS_DLOG(ERROR, result)
      << "Failed to set current device for AU, element:" << element
      << " voice: " << apply_voice_processing;
  DestroyAudioUnit(audio_unit, element_, reenable_io_);
}

ScopedAudioUnit::~ScopedAudioUnit() {
  DestroyAudioUnit(audio_unit_, element_, reenable_io_);
  LOG(ERROR) << __FUNCTION__;
}

}  // namespace media
