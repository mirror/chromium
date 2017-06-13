// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletNode.h"

#include "bindings/core/v8/ExceptionMessages.h"
#include "bindings/core/v8/ExceptionState.h"
#include "modules/webaudio/AudioBuffer.h"
#include "modules/webaudio/AudioNodeInput.h"
#include "modules/webaudio/AudioNodeOutput.h"
#include "modules/webaudio/AudioParamDescriptor.h"
#include "modules/webaudio/AudioParamMap.h"
#include "modules/webaudio/AudioWorkletProcessor.h"
#include "modules/webaudio/AudioWorkletProcessorDefinition.h"
#include "modules/webaudio/AudioWorkletNodeOptions.h"
#include "platform/audio/AudioBus.h"
#include "platform/audio/AudioUtilities.h"

namespace blink {

AudioWorkletHandler::AudioWorkletHandler(
    AudioNode& node,
    float sample_rate,
    const AudioWorkletProcessorDefinition* definition,
    unsigned long number_of_inputs,
    unsigned long number_of_outputs)
    : AudioHandler(kNodeTypeAudioWorklet, node, sample_rate) {
  DCHECK(IsMainThread());

  for (unsigned input = 0; input < number_of_inputs; ++input)
    AddInput();

  for (unsigned output = 0; output < number_of_outputs; ++output)
    AddOutput(1);

  Initialize();
}

AudioWorkletHandler::~AudioWorkletHandler() {
  definition_.reset();
  processor_.reset();
  Uninitialize();
}

PassRefPtr<AudioWorkletHandler> AudioWorkletHandler::Create(
    AudioNode& node,
    float sample_rate,
    const AudioWorkletProcessorDefinition* definition,
    unsigned long number_of_inputs,
    unsigned long number_of_outputs) {
  return AdoptRef(new AudioWorkletHandler(
      node, sample_rate, definition, number_of_inputs, number_of_outputs));
}

void AudioWorkletHandler::Process(size_t frames_to_process) {

}

// ----------------------------------------------------------------

AudioWorkletNode::AudioWorkletNode(
    BaseAudioContext& context,
    const AudioWorkletProcessorDefinition* definition,
    const AudioWorkletNodeOptions& options)
    : AudioNode(context) {
  InitializeAudioParamMap(definition, options);
  SetHandler(AudioWorkletHandler::Create(*this, context.sampleRate(),
      definition, options.numberOfInputs(), options.numberOfOutputs()));
}

AudioWorkletNode* AudioWorkletNode::Create(
    BaseAudioContext* context,
    const String& name,
    ExceptionState& exception_state) {
  DCHECK(IsMainThread());

  if (context->IsContextClosed()) {
    context->ThrowExceptionForClosedState(exception_state);
    return nullptr;
  }

  // TODO: the getter isn't implemented in BaseAudioContext. (won't compile)
  AudioWorkletProcessorDefinition* definition =
      context->GetWorkletMessagingProxy()->GetProcessorDefinition(name);
  if (!definition) {
    exception_state.ThrowDOMException(
        kNotFoundError,
        "Undefined AudioWorkletNode name: " + name);
    return nullptr;
  }

  AudioWorkletNodeOptions options;
  AudioWorkletNode* node = new AudioWorkletNode(*context, definition, options);
  if (!node)
    return nullptr;

  return node;
}

AudioWorkletNode* AudioWorkletNode::Create(
    BaseAudioContext* context,
    const String& name,
    const AudioWorkletNodeOptions& options,
    ExceptionState& exception_state) {
  DCHECK(IsMainThread());

  if (context->IsContextClosed()) {
    context->ThrowExceptionForClosedState(exception_state);
    return nullptr;
  }

  // TODO: the getter isn't implemented in BaseAudioContext. (won't compile)
  AudioWorkletProcessorDefinition* definition =
      context->GetWorkletMessagingProxy()->GetProcessorDefinition(name);
  if (!definition) {
    exception_state.ThrowDOMException(
        kNotFoundError,
        "Undefined AudioWorkletNode name: " + name);
    return nullptr;
  }

  AudioWorkletNode* node = new AudioWorkletNode(*context, definition, options);
  if (!node)
    return nullptr;

  node->HandleChannelOptions(options, exception_state);

  return node;
}

AudioParamMap* AudioWorkletNode::parameters() const {
  return parameter_map_;
}

bool AudioWorkletNode::HasPendingActivity() const {
  return !context()->IsContextClosed();
}

AudioWorkletHandler& AudioWorkletNode::GetWorkletHandler() const {
  return static_cast<AudioWorkletHandler&>(Handler());
}

void AudioWorkletNode::InitializeAudioParamMap(
    const AudioWorkletProcessorDefinition* definition,
    const AudioWorkletNodeOptions& options) {
  DCHECK(IsMainThread());

  HeapHashMap<String, Member<AudioParam>> audio_param_map;

  // TODO(hongchan): |definition| belongs to Worklet thread, and this might not
  // be thread-safe. Instead, do access the cloned |definition| in
  // |AudioWorklet|.
  for (const auto& descriptor : definition->GetAudioParamDescriptors()) {
    AudioParam* audio_param = AudioParam::Create(*context(),
                                                 kParamTypeAudioWorkletNode,
                                                 descriptor.defaultValue(),
                                                 descriptor.minValue(),
                                                 descriptor.maxValue());
    DCHECK(audio_param);
    audio_param_map.Set(descriptor.name().IsolatedCopy(), audio_param);
  }

  // Set the initial parameter value if it exists in the options.
  if (options.hasParameterData()) {
    for (const auto& param_data : options.parameterData()) {
      if (audio_param_map.Contains(param_data.first))
        audio_param_map.at(param_data.first)->setValue(param_data.second);
    }
  }

  parameter_map_ = new AudioParamMap(audio_param_map);
}

DEFINE_TRACE(AudioWorkletNode) {
  visitor->Trace(parameter_map_);
  AudioNode::Trace(visitor);
}

}  // namespace blink
