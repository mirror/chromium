// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorklet_h
#define AudioWorklet_h

#include "core/workers/Worklet.h"
#include "modules/ModulesExport.h"
#include "platform/heap/Handle.h"

namespace blink {

class AudioWorkletMessagingProxy;
class BaseAudioContext;

class MODULES_EXPORT AudioWorklet final : public Worklet {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(AudioWorklet);
  WTF_MAKE_NONCOPYABLE(AudioWorklet);

 public:
  static AudioWorklet* Create(BaseAudioContext*);

  ~AudioWorklet();

  // This can return nullptr if audioWorklet.addModule() has not invoked yet.
  AudioWorkletMessagingProxy* GetWorkletMessagingProxy();

  // Implements Worklet
  void Trace(blink::Visitor*) override;

 private:
  explicit AudioWorklet(BaseAudioContext*);

  // Implements Worklet
  bool NeedsToCreateGlobalScope() final;
  WorkletGlobalScopeProxy* CreateGlobalScope() final;

  Member<BaseAudioContext> context_;
};

}  // namespace blink

#endif  // AudioWorklet_h
