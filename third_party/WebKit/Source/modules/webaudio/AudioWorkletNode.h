// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorkletNode_h
#define AudioWorkletNode_h

#include "modules/webaudio/AudioNode.h"
#include "platform/wtf/PassRefPtr.h"

namespace blink {

class AudioParamMap;
class AudioWorkletNodeOptions;
class AudioWorkletProcessor;
class AudioWorkletProcessorDefinition;
class BaseAudioContext;
class ExceptionState;

class AudioWorkletHandler final : public AudioHandler {
 public:
  static PassRefPtr<AudioWorkletHandler> Create(
      AudioNode&,
      float sample_rate,
      const AudioWorkletProcessorDefinition*,
      unsigned long number_of_inputs,
      unsigned long number_of_outputs);

  ~AudioWorkletHandler() override;

  void Process(size_t frames_to_process) override;

 private:
  AudioWorkletHandler(
      AudioNode&,
      float sample_rate,
      const AudioWorkletProcessorDefinition*,
      unsigned long number_of_inputs,
      unsigned long number_of_outputs);

  std::unique_ptr<AudioWorkletProcessor> processor_;
  std::unique_ptr<AudioWorkletProcessorDefinition> definition_;
};

class AudioWorkletNode final : public AudioNode,
                               public ActiveScriptWrappable<AudioWorkletNode> {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(AudioWorkletNode);

 public:
  static AudioWorkletNode* Create(BaseAudioContext*,
                                  const String& name,
                                  ExceptionState&);
  static AudioWorkletNode* Create(BaseAudioContext*,
                                  const String& name,
                                  const AudioWorkletNodeOptions&,
                                  ExceptionState&);

  // IDL attribute
  AudioParamMap* parameters() const;

  // ScriptWrappable
  bool HasPendingActivity() const final;

  AudioWorkletHandler& GetWorkletHandler() const;

  DECLARE_VIRTUAL_TRACE();

 private:
  AudioWorkletNode(BaseAudioContext&,
                   const AudioWorkletProcessorDefinition*,
                   const AudioWorkletNodeOptions&);

  void InitializeAudioParamMap(const AudioWorkletProcessorDefinition*,
                               const AudioWorkletNodeOptions&);

  Member<AudioParamMap> parameter_map_;
};

}  // namespace blink

#endif  // AudioWorkletNode_h
