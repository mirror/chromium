// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaCapabilitiesController_h
#define MediaCapabilitiesController_h

#include "core/dom/ContextLifecycleObserver.h"
#include "modules/ModulesExport.h"
#include "platform/Supplementable.h"

namespace blink {

class WebMediaCapabilitiesClient;

class MODULES_EXPORT MediaCapabilitiesController final
    : public GarbageCollected<MediaCapabilitiesController>,
      public Supplement<LocalFrame>,
      public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(MediaCapabilitiesController);
  WTF_MAKE_NONCOPYABLE(MediaCapabilitiesController);

 public:
  static void ProvideTo(LocalFrame&, WebMediaCapabilitiesClient*);

  static const char* SupplementName();
  static MediaCapabilitiesController* From(LocalFrame&);

  // ContextLifecycleObserver
  void ContextDestroyed(ExecutionContext*) override;

  WebMediaCapabilitiesClient* Client();

  DECLARE_VIRTUAL_TRACE();

 private:
  MediaCapabilitiesController(LocalFrame&, WebMediaCapabilitiesClient*);

  WebMediaCapabilitiesClient* client_ = nullptr;
};

}  // namespace blink

#endif  // MediaCapabilitiesController_h
