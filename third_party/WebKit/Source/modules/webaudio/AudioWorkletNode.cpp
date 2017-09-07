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
#include "modules/webaudio/CrossThreadAudioWorkletProcessorInfo.h"
#include "platform/audio/AudioBus.h"
#include "platform/audio/AudioUtilities.h"
#include "platform/heap/Persistent.h"

namespace blink {

AudioWorkletHandler::AudioWorkletHandler(
    AudioNode& node,
    float sample_rate,
    String name,
    HashMap<String, RefPtr<AudioParamHandler>> param_handler_map,
    const AudioWorkletNodeOptions& options)
    : AudioHandler(kNodeTypeAudioWorklet, node, sample_rate),
      name_(name) {
  DCHECK(IsMainThread());

  for (unsigned i = 0; i < options.numberOfInputs(); ++i)
    AddInput();
  for (unsigned i = 0; i < options.numberOfOutputs(); ++i)
    AddOutput(1);
}

AudioWorkletHandler::~AudioWorkletHandler() {
  Uninitialize();
}

PassRefPtr<AudioWorkletHandler> AudioWorkletHandler::Create(
    AudioNode& node,
    float sample_rate,
    String name,
    HashMap<String, RefPtr<AudioParamHandler>> param_handler_map,
    const AudioWorkletNodeOptions& options) {
  return AdoptRef(new AudioWorkletHandler(
      node, sample_rate, name, param_handler_map, options));
}

void AudioWorkletHandler::Process(size_t frames_to_process) {
  AudioBus* output_bus = Output(0).Bus();

  if (!processor_ || !IsInitialized()) {
    output_bus->Zero();
    return;
  }

  output_bus->Zero();

  // When the lifecycle of |AudioWorkletProcessor| is over.
  // processor_ = nullptr;
}

double AudioWorkletHandler::TailTime() const {
  return std::numeric_limits<double>::infinity();
}

void AudioWorkletHandler::SetProcessorOnRenderingThread(
    AudioWorkletProcessor* processor) {
  DCHECK(!IsMainThread());
  processor_ = processor;
}

// ----------------------------------------------------------------

AudioWorkletNode::AudioWorkletNode(
    BaseAudioContext& context,
    const String& name,
    const AudioWorkletNodeOptions& options,
    const Vector<CrossThreadAudioParamInfo> param_info_list)
    : AudioNode(context) {

  HeapHashMap<String, Member<AudioParam>> audio_param_map;
  HashMap<String, RefPtr<AudioParamHandler>> param_handler_map;
  for (const auto& param_info : param_info_list) {
    String param_name = param_info.Name().IsolatedCopy();
    AudioParam* audio_param = AudioParam::Create(context,
                                                 kParamTypeAudioWorklet,
                                                 param_info.DefaultValue(),
                                                 param_info.MinValue(),
                                                 param_info.MaxValue());
    audio_param_map.Set(param_name, audio_param);
    param_handler_map.Set(param_name, audio_param->HandlerRefPtr());

    // TODO(hongchan): set the initial value with |options|.
  }
  parameter_map_ = new AudioParamMap(audio_param_map);

  SetHandler(AudioWorkletHandler::Create(*this,
                                         context.sampleRate(),
                                         name,
                                         param_handler_map,
                                         options));
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

  AudioWorkletMessagingProxy* proxy = context->WorkletMessagingProxy();

  if (!proxy) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "AudioWorkletNode cannot be created: AudioWorklet does not have a "
        "valid AudioWorkletGlobalScope.");
    return nullptr;
  }

  if (!proxy->IsProcessorRegistered(name)) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "AudioWorkletNode cannot be created: The node name '" + name +
        "' is not defined in AudioWorkletGlobalScope.");
    return nullptr;
  }

  AudioWorkletNode* node = new AudioWorkletNode(
      *context, name, options, proxy->GetParamInfoListForProcessor(name));

  if (!node) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "AudioWorkletNode cannot be created.");
    return nullptr;
  }

  // This is non-blocking async call. |node| still can be returned to user.
  proxy->CreateProcessor(&node->GetWorkletHandler());

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
