// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioParamMap.h"

namespace blink {

class AudioParamMapIterationSource final
    : public PairIterable<String, AudioParam*>::IterationSource {
 public:
  AudioParamMapIterationSource(
      const HeapHashMap<String, Member<AudioParam>>& map)
      : map_(map) {
    for (const auto name : map_->Keys()) {
      parameter_names_.push_back(name);
    }
  }

  bool Next(ScriptState* scrip_state,
            String& key,
            AudioParam*& audio_param,
            ExceptionState&) override {
    if (current_index_ == map_->size())
      return false;
    key = parameter_names_[current_index_];
    audio_param = map_->at(key);
    ++current_index_;
    return true;
  }

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(map_);
    PairIterable<String, AudioParam*>::IterationSource::Trace(visitor);
  }

 private:
  // Hold the map as a member just to keep it alive while there is an active
  // iterator to it.
  const Member<const HeapHashMap<String, Member<AudioParam>>> map_;

  // For sequential iteration (e.g. Next()), the key strings and the index are
  // necessary.
  Vector<String> parameter_names_;
  unsigned current_index_;
};

AudioParamMap::AudioParamMap(
    const HeapHashMap<String, Member<AudioParam>>& parameter_map)
    : parameter_map_(parameter_map) {}

PairIterable<String, AudioParam*>::IterationSource*
    AudioParamMap::StartIteration(ScriptState*, ExceptionState&) {
  return new AudioParamMapIterationSource(parameter_map_);
}

bool AudioParamMap::GetMapEntry(ScriptState*,
                                const String& key,
                                AudioParam*& audio_param,
                                ExceptionState&) {
  if (parameter_map_.Contains(key)) {
    audio_param = parameter_map_.at(key);
    return true;
  }

  return false;
}

}  // namespace blink
