// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorkletProcessorInfo_h
#define AudioWorkletProcessorInfo_h

#include "modules/webaudio/AudioParamDescriptor.h"
#include "modules/webaudio/AudioWorkletProcessorDefinition.h"

namespace blink {

class AudioParamInfo {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  AudioParamInfo(const AudioParamDescriptor* descriptor)
      : name_(descriptor->name()),
        default_value_(descriptor->defaultValue()),
        max_value_(descriptor->maxValue()),
        min_value_(descriptor->minValue()) {}

  const String& GetName() { return name_; }
  float GetDefaultValue() { return default_value_; }
  float GetMaxValue() { return max_value_; }
  float GetMinValue() { return min_value_; }

 private:
  const String name_;
  const float default_value_;
  const float max_value_;
  const float min_value_;
};

class AudioWorkletProcessorInfo {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  AudioWorkletProcessorInfo(const AudioWorkletProcessorDefinition* definition)
      : name_(definition->GetName()) {
    for (const String& name : definition->GetAudioParamDescriptorNames()) {
      audio_param_infos_.emplace_back(
          definition->GetAudioParamDescriptor(name));
    }
  }

  const String& GetName() { return name_; }
  Vector<AudioParamInfo> GetAudioParamInfo() { return audio_param_infos_; }

 private:
  const String name_;
  Vector<AudioParamInfo> audio_param_infos_;
};

}  // namespace blink

#endif  // AudioWorkletProcessorInfo_h
