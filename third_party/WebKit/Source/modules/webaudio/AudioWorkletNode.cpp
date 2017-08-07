// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletNode.h"

#include "core/dom/TaskRunnerHelper.h"
#include "modules/webaudio/AudioBuffer.h"
#include "modules/webaudio/AudioNodeInput.h"
#include "modules/webaudio/AudioNodeOutput.h"
#include "modules/webaudio/AudioParamDescriptor.h"
#include "modules/webaudio/AudioWorkletGlobalScope.h"
#include "modules/webaudio/AudioWorkletMessagingProxy.h"
#include "modules/webaudio/AudioWorkletProcessor.h"
#include "modules/webaudio/AudioWorkletProcessorDefinition.h"
#include "platform/audio/AudioBus.h"
#include "platform/audio/AudioUtilities.h"
#include "platform/heap/Persistent.h"

namespace blink {

AudioWorkletHandler::AudioWorkletHandler(AudioNode& node,
                                         float sample_rate,
                                         String name,
                                         unsigned number_of_inputs,
                                         unsigned number_of_outputs)
    : AudioHandler(kNodeTypeAudioWorklet, node, sample_rate),
      name_(name) {
  DCHECK(IsMainThread());

  for (unsigned i = 0; i < number_of_inputs; ++i)
    AddInput();

  for (unsigned i = 0; i < number_of_outputs; ++i)
    AddOutput(1);
}

AudioWorkletHandler::~AudioWorkletHandler() {
  Uninitialize();
  processor_ = nullptr;
}

PassRefPtr<AudioWorkletHandler> AudioWorkletHandler::Create(
    AudioNode& node,
    float sample_rate,
    String name,
    unsigned number_of_inputs,
    unsigned number_of_outputs) {
  return AdoptRef(new AudioWorkletHandler(
      node, sample_rate, name, number_of_inputs, number_of_outputs));
}

void AudioWorkletHandler::Process(size_t frames_to_process) {
  AudioBus* output_bus = Output(0).Bus();
  DCHECK(output_bus);
}

double AudioWorkletHandler::TailTime() const {
  return std::numeric_limits<double>::infinity();
}

// ----------------------------------------------------------------

AudioWorkletNode::AudioWorkletNode(BaseAudioContext& context,
                                   const String& name,
                                   const AudioWorkletNodeOptions& options)
    : AudioNode(context), node_options_(options) {
  SetHandler(AudioWorkletHandler::Create(*this,
                                         context.sampleRate(),
                                         name,
                                         options.numberOfInputs(),
                                         options.numberOfOutputs()));
  context.GetWorkletMessagingProxy()->CreateProcessor(&GetWorkletHandler());
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

  AudioWorkletNode* node = new AudioWorkletNode(*context, name, options);

  if (!node || !node->GetWorkletHandler().IsInitialized())
    return nullptr;

  // Do something with node.
  node->HandleChannelOptions(options, exception_state);

  // context keeps reference as a source node.
  // context->NotifySourceNodeStartedProcessing(node);

  return node;
}

bool AudioWorkletNode::HasPendingActivity() const {
  return !context()->IsContextClosed();
}

AudioParamMap* AudioWorkletNode::parameters() const {
  return parameter_map_;
}

AudioWorkletHandler& AudioWorkletNode::GetWorkletHandler() const {
  return static_cast<AudioWorkletHandler&>(Handler());
}

DEFINE_TRACE(AudioWorkletNode) {
  visitor->Trace(parameter_map_);
  AudioNode::Trace(visitor);
}

}  // namespace blink
