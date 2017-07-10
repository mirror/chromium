// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorklet.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/workers/WorkerClients.h"
#include "modules/webaudio/AudioWorkletMessagingProxy.h"
#include "modules/webaudio/AudioWorkletThread.h"
#include "modules/webaudio/BaseAudioContext.h"

namespace blink {

AudioWorklet* AudioWorklet::Create(LocalFrame* frame) {
  return new AudioWorklet(frame);
}

AudioWorklet::AudioWorklet(LocalFrame* frame) : Worklet(frame) {}

AudioWorklet::~AudioWorklet() {
  base_audio_contexts_.clear();
}

void AudioWorklet::RegisterBaseAudioContext(BaseAudioContext* context) {
  DCHECK(IsMainThread());
  DCHECK(!base_audio_contexts_.Contains(context));
  base_audio_contexts_.push_back(context);
}

void AudioWorklet::UnregisterBaseAudioContext(BaseAudioContext* context) {
  DCHECK(IsMainThread());
  size_t index = base_audio_contexts_.Find(context);
  if (index == kNotFound)
    return;

  // TODO(hongchan): Rearrange the vector data after the deletion. (shrink/fit)
  base_audio_contexts_.erase(index);
}

bool AudioWorklet::NeedsToCreateGlobalScope() {
  return GetNumberOfGlobalScopes() == 0 ||
      GetNumberOfGlobalScopes() < base_audio_contexts_.size();
}

WorkletGlobalScopeProxy* AudioWorklet::CreateGlobalScope() {
  DCHECK(NeedsToCreateGlobalScope());

  AudioWorkletThread::EnsureSharedBackingThread();

  WorkerClients* worker_clients = WorkerClients::Create();
  AudioWorkletMessagingProxy* proxy =
      new AudioWorkletMessagingProxy(GetExecutionContext(), worker_clients);
  proxy->Initialize();

  return proxy;
}

DEFINE_TRACE(AudioWorklet) {
  visitor->Trace(base_audio_contexts_);
  Worklet::Trace(visitor);
}

}  // namespace blink
