// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletProcessor.h"

#include "core/dom/MessagePort.h"
#include "modules/webaudio/AudioBuffer.h"
#include "modules/webaudio/AudioWorkletGlobalScope.h"

namespace blink {

// This static factory should be called after an instance of |AudioWorkletNode|
// gets created by user-supplied JS code in the main thread. This factory must
// not be called by user in |AudioWorkletGlobalScope|.
AudioWorkletProcessor* AudioWorkletProcessor::Create(
    ExecutionContext* context) {
  AudioWorkletGlobalScope* global_scope = ToAudioWorkletGlobalScope(context);
  DCHECK(global_scope);
  DCHECK(global_scope->IsContextThread());
  return new AudioWorkletProcessor(global_scope);
}

AudioWorkletProcessor::AudioWorkletProcessor(
    AudioWorkletGlobalScope* global_scope)
    : global_scope_(global_scope) {}

AudioWorkletProcessor::~AudioWorkletProcessor() {}

// void AudioWorkletProcessor::SetInstance(v8::Isolate* isolate,
//                                         v8::Local<v8::Object> instance) {
//   DCHECK(global_scope_->IsContextThread());
//   instance_.Set(isolate, instance);
// }

// v8::Local<v8::Object> AudioWorkletProcessor::InstanceLocal(
//     v8::Isolate* isolate) {
//   DCHECK(global_scope_->IsContextThread());
//   return instance_.NewLocal(isolate);
// }

bool AudioWorkletProcessor::Process(
    Vector<AudioBus*>* input_buses,
    Vector<AudioBus*>* output_buses,
    HashMap<String, std::unique_ptr<AudioFloatArray>>* param_value_map,
    double current_time) {
  DCHECK(global_scope_->IsContextThread());
  return global_scope_->Process(
      this, input_buses, output_buses, param_value_map, current_time);
}

MessagePort* AudioWorkletProcessor::port() const {
  return processor_port_;
}

void AudioWorkletProcessor::Trace(blink::Visitor* visitor) {
  visitor->Trace(global_scope_);
  visitor->Trace(processor_port_);
  ScriptWrappable::Trace(visitor);
}

// void AudioWorkletProcessor::TraceWrappers(
//     const ScriptWrappableVisitor* visitor) const {
//   visitor->TraceWrappers(instance_.Cast<v8::Value>());
// }

}  // namespace blink
