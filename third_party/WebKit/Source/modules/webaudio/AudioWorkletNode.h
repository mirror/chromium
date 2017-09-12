// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorkletNode_h
#define AudioWorkletNode_h

#include "modules/webaudio/AudioNode.h"
#include "modules/webaudio/AudioParamMap.h"
#include "modules/webaudio/AudioWorkletNodeOptions.h"
#include "platform/wtf/PassRefPtr.h"
#include "platform/wtf/Threading.h"

namespace blink {

class AudioWorkletProcessor;
class BaseAudioContext;
class CrossThreadAudioParamInfo;
class ExceptionState;

class AudioWorkletHandler final : public AudioHandler {
 public:
  static PassRefPtr<AudioWorkletHandler> Create(
      AudioNode&,
      float sample_rate,
      String name,
      HashMap<String, RefPtr<AudioParamHandler>> param_handler_map,
      const AudioWorkletNodeOptions&);

  ~AudioWorkletHandler() override;

  // AudioHandler
  void Process(size_t frames_to_process) override;

  double TailTime() const override;
  double LatencyTime() const override { return 0; }

  const String& Name() const { return name_; }

  // Sets |AudioWorkletProcessor| on the rendering thread.
  void SetProcessorOnRenderingThread(AudioWorkletProcessor*);

 private:
  AudioWorkletHandler(
      AudioNode&,
      float sample_rate,
      String name,
      HashMap<String, RefPtr<AudioParamHandler>> param_handler_map,
      const AudioWorkletNodeOptions&);

  // This is set/used by the rendering thread.
  UntracedMember<AudioWorkletProcessor> processor_;

  const String name_;
  HashMap<String, RefPtr<AudioParamHandler>> param_handler_map_;
  HashMap<String, std::unique_ptr<AudioFloatArray>> param_value_map_;
};

class AudioWorkletNode final : public AudioNode,
                               public ActiveScriptWrappable<AudioWorkletNode> {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(AudioWorkletNode);

 public:
  static AudioWorkletNode* Create(BaseAudioContext*,
                                  const String& name,
                                  const AudioWorkletNodeOptions&,
                                  ExceptionState&);

  AudioWorkletHandler& GetWorkletHandler() const;

  // ScriptWrappable
  bool HasPendingActivity() const final;

  // IDL
  AudioParamMap* parameters() const;

  DECLARE_VIRTUAL_TRACE();

 private:
  AudioWorkletNode(BaseAudioContext&,
                   const String& name,
                   const AudioWorkletNodeOptions&,
                   const Vector<CrossThreadAudioParamInfo>);

  AudioWorkletNodeOptions node_options_;
  Member<AudioParamMap> parameter_map_;
};

}  // namespace blink

#endif  // AudioWorkletNode_h
