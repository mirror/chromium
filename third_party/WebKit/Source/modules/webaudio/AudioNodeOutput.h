/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef AudioNodeOutput_h
#define AudioNodeOutput_h

#include <memory>
#include "modules/webaudio/AudioNode.h"
#include "modules/webaudio/AudioParam.h"
#include "platform/audio/AudioBus.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class AudioNodeInput;

// AudioNodeOutput represents a single output for an AudioNode.
// It may be connected to one or more AudioNodeInputs.
class AudioNodeOutput final {
  USING_FAST_MALLOC(AudioNodeOutput);

 public:
  // It's OK to pass 0 for numberOfChannels in which case
  // SetNumberOfChannels() must be called later on.
  static std::unique_ptr<AudioNodeOutput> Create(AudioHandler*,
                                                 unsigned number_of_channels);
  void Dispose();

  // Causes our AudioNode to process if it hasn't already for this render
  // quantum.  It returns the bus containing the processed audio for this
  // output, returning in_place_bus if in-place processing was possible.  Called
  // from context's audio thread.
  AudioBus* Pull(AudioBus* in_place_bus, size_t frames_to_process);

  // Bus() will contain the rendered audio after Pull() is called for each
  // rendering time quantum.
  // Called from context's audio thread.
  AudioBus* Bus() const;

  // RenderingFanOutCount() is the number of AudioNodeInputs that we're
  // connected to during rendering.  Unlike FanOutCount() it will not change
  // during the course of a render quantum.
  unsigned RenderingFanOutCount() const;

  // Must be called with the context's graph lock.
  void DisconnectAll();

  // Disconnect a specific input or AudioParam.
  void DisconnectInput(AudioNodeInput&);
  void DisconnectAudioParam(AudioParamHandler&);

  void SetNumberOfChannels(unsigned);
  unsigned NumberOfChannels() const { return number_of_channels_; }
  bool IsChannelCountKnown() const { return NumberOfChannels() > 0; }

  bool IsConnected() { return FanOutCount() > 0 || ParamFanOutCount() > 0; }

  // Probe if the output node is connected with a certain input or AudioParam
  bool IsConnectedToInput(AudioNodeInput&);
  bool IsConnectedToAudioParam(AudioParamHandler&);

  // Disable/Enable happens when there are still JavaScript references to a
  // node, but it has otherwise "finished" its work.  For example, when a note
  // has finished playing.  It is kept around, because it may be played again at
  // a later time.  They must be called with the context's graph lock.
  void Disable();
  void Enable();

  // UpdateRenderingState() is called in the audio thread at the start or end of
  // the render quantum to handle any recent changes to the graph state.
  // It must be called with the context's graph lock.
  void UpdateRenderingState();

 private:
  AudioNodeOutput(AudioHandler*, unsigned number_of_channels);
  // Can be called from any thread.
  AudioHandler& Handler() const { return handler_; }
  DeferredTaskHandler& GetDeferredTaskHandler() const {
    return handler_.Context()->GetDeferredTaskHandler();
  }

  // This reference is safe because the AudioHandler owns this AudioNodeOutput
  // object.
  AudioHandler& handler_;

  friend class AudioNodeInput;
  friend class AudioParamHandler;

  // These are called from AudioNodeInput.
  // They must be called with the context's graph lock.
  void AddInput(AudioNodeInput&);
  void RemoveInput(AudioNodeInput&);
  void AddParam(AudioParamHandler&);
  void RemoveParam(AudioParamHandler&);

  // FanOutCount() is the number of AudioNodeInputs that we're connected to.
  // This method should not be called in audio thread rendering code, instead
  // RenderingFanOutCount() should be used.
  // It must be called with the context's graph lock.
  unsigned FanOutCount();

  // Similar to FanOutCount(), ParamFanOutCount() is the number of AudioParams
  // that we're connected to.  This method should not be called in audio thread
  // rendering code, instead RenderingParamFanOutCount() should be used.
  // It must be called with the context's graph lock.
  unsigned ParamFanOutCount();

  // Must be called with the context's graph lock.
  void DisconnectAllInputs();
  void DisconnectAllParams();

  // UpdateInternalBus() updates internal_bus_ appropriately for the number of
  // channels.  It is called in the constructor or in the audio thread with the
  // context's graph lock.
  void UpdateInternalBus();

  // Announce to any nodes we're connected to that we changed our channel count
  // for its input.
  // It must be called in the audio thread with the context's graph lock.
  void PropagateChannelCount();

  // UpdateNumberOfChannels() is called in the audio thread at the start or end
  // of the render quantum to pick up channel changes.
  // It must be called with the context's graph lock.
  void UpdateNumberOfChannels();

  // number_of_channels_ will only be changed in the audio thread.
  // The main thread sets desired_number_of_channels_ which will later get
  // picked up in the audio thread in UpdateNumberOfChannels().
  unsigned number_of_channels_;
  unsigned desired_number_of_channels_;

  // internal_bus_ and in_place_bus_ must only be changed in the audio thread
  // with the context's graph lock (or constructor).
  RefPtr<AudioBus> internal_bus_;
  RefPtr<AudioBus> in_place_bus_;
  // If is_in_place_ is true, use in_place_bus_ as the valid AudioBus; If false,
  // use the default internal_bus_.
  bool is_in_place_;

  // This HashSet holds connection references. We must call
  // AudioNode::MakeConnection when we add an AudioNodeInput to this, and must
  // call AudioNode::BreakConnection() when we remove an AudioNodeInput from
  // this.
  HashSet<AudioNodeInput*> inputs_;
  bool is_enabled_;

  bool did_call_dispose_;

  // For the purposes of rendering, keeps track of the number of inputs and
  // AudioParams we're connected to.  These value should only be changed at the
  // very start or end of the rendering quantum.
  unsigned rendering_fan_out_count_;
  unsigned rendering_param_fan_out_count_;

  // This collection of raw pointers is safe because they are retained by
  // AudioParam objects retained by connected_params_ of the owner AudioNode.
  HashSet<AudioParamHandler*> params_;
};

}  // namespace blink

#endif  // AudioNodeOutput_h
