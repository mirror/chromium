// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorklet.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/Document.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/workers/WorkerClients.h"
#include "modules/webaudio/AudioWorkletMessagingProxy.h"
#include "modules/webaudio/BaseAudioContext.h"

namespace blink {

AudioWorklet* AudioWorklet::Create(BaseAudioContext* context) {
  return new AudioWorklet(context);
}

// TODO(hongchan): Worklet is inherently document-dependent, which is not
// compatible with WebWorker.
AudioWorklet::AudioWorklet(BaseAudioContext* context)
    : Worklet(context->GetExecutionContext()->ExecutingWindow()->GetFrame()),
      context_(context) {}

AudioWorklet::~AudioWorklet() {}

AudioWorkletMessagingProxy* AudioWorklet::GetWorkletMessagingProxy() {
  return GetNumberOfGlobalScopes() == 0
      ? nullptr
      : static_cast<AudioWorkletMessagingProxy*>(FindAvailableGlobalScope());
}

bool AudioWorklet::NeedsToCreateGlobalScope() {
  // Each BaseAudioContext can only have one AudioWorkletGlobalScope.
  return GetNumberOfGlobalScopes() == 0;
}

WorkletGlobalScopeProxy* AudioWorklet::CreateGlobalScope() {
  DCHECK(NeedsToCreateGlobalScope());

  WorkerClients* worker_clients = WorkerClients::Create();
  AudioWorkletMessagingProxy* proxy =
      new AudioWorkletMessagingProxy(GetExecutionContext(), worker_clients);
  proxy->Initialize();

  // Notify BaseAudioContext that AudioWorkletMessagingProxy and GlobalScope
  // have been created.
  context_->SetWorkletMessagingProxy(proxy);

  return proxy;
}

void AudioWorklet::Trace(blink::Visitor* visitor) {
  visitor->Trace(context_);
  Worklet::Trace(visitor);
}

}  // namespace blink
