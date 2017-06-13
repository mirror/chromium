// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioParamMap_h
#define AudioParamMap_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/Maplike.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "modules/webaudio/AudioParam.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class AudioParamMap final : public GarbageCollectedFinalized<AudioParamMap>,
                            public ScriptWrappable,
                            public Maplike<String, AudioParam*> {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit AudioParamMap(
      const HeapHashMap<String, Member<AudioParam>>& parameter_map)
      : parameter_map_(parameter_map) {}

  // IDL attributes / methods
  size_t size() const { return parameter_map_.size(); }

  AudioParam* at(String param_name) {
    return parameter_map_.at(param_name);
  }

  bool Contains(String param_name) {
    return parameter_map_.Contains(param_name);
  }

  DEFINE_INLINE_VIRTUAL_TRACE() { visitor->Trace(parameter_map_); }

 private:
  using ParameterMap = HeapHashMap<String, Member<AudioParam>>;
  using MapIterator = typename ParameterMap::const_iterator;

  typename PairIterable<String, AudioParam*>::IterationSource* StartIteration(
      ScriptState*,
      ExceptionState&) override {
    return new AudioParamMapIterationSource(parameter_map_);
  }

  bool GetMapEntry(ScriptState*,
                   const String& key,
                   AudioParam*& audio_param,
                   ExceptionState&) override {
    if (parameter_map_.Contains(key)) {
      audio_param = parameter_map_.at(key);
      return true;
    }

    return false;
  }

  class AudioParamMapIterationSource final
      : public PairIterable<String, AudioParam*>::IterationSource {
   public:
    AudioParamMapIterationSource(const ParameterMap& map)
        : map_(map) {
      for (const auto name : map_->Keys()) {
        parameter_names_.push_back(name);
      }
    }

    // TODO(hongchan): fix this with proper map iterators.
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
    const Member<const ParameterMap> map_;
    Vector<String> parameter_names_;
    unsigned current_index_;
  };

  const ParameterMap parameter_map_;
};

}  // namespace blink

#endif
