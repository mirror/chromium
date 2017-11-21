// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorklet_h
#define AudioWorklet_h

#include "core/workers/Worklet.h"
#include "modules/ModulesExport.h"
#include "platform/heap/Handle.h"

namespace blink {

class AudioWorkletHandler;
class AudioWorkletMessagingProxy;
class BaseAudioContext;
class CrossThreadAudioParamInfo;
class MessagePortChannel;

class MODULES_EXPORT AudioWorklet final : public Worklet {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(AudioWorklet);
  WTF_MAKE_NONCOPYABLE(AudioWorklet);

 public:
  // When the runtime flag is not enabled, this constructor returns |nullptr|.
  static AudioWorklet* Create(BaseAudioContext*);

  ~AudioWorklet();

  void CreateProcessor(AudioWorkletHandler*, MessagePortChannel);
  WebThread* GetBackingThread();
  const Vector<CrossThreadAudioParamInfo> GetParamInfoListForProcessor(
      const String& name);
  bool IsProcessorRegistered(const String& name);
  bool IsReady();

  void Trace(blink::Visitor*) override;

 private:
  explicit AudioWorklet(BaseAudioContext*);

  // Implements Worklet
  bool NeedsToCreateGlobalScope() final;
  WorkletGlobalScopeProxy* CreateGlobalScope() final;

  // Returns |nullptr| if there is no active WorkletGlobalScope().
  AudioWorkletMessagingProxy* GetMessagingProxy();

  Member<BaseAudioContext> context_;
};

}  // namespace blink

#endif  // AudioWorklet_h
