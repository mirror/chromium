// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebDevToolsAgentCore_h
#define WebDevToolsAgentCore_h

#include "core/CoreExport.h"
#include "platform/heap/Heap.h"
#include "public/web/WebDevToolsAgent.h"

namespace blink {

class CORE_EXPORT WebDevToolsAgentCore
    : public GarbageCollectedFinalized<WebDevToolsAgentCore>,
      public NON_EXPORTED_BASE(WebDevToolsAgent) {
 public:
  virtual ~WebDevToolsAgentCore() {}
  DEFINE_INLINE_VIRTUAL_TRACE();

  virtual void WillBeDestroyed() = 0;
  virtual WebDevToolsAgentClient* Client() = 0;
  virtual void FlushProtocolNotifications() = 0;
  virtual void PaintOverlay() = 0;
  virtual void LayoutOverlay() = 0;
  virtual bool HandleInputEvent(const WebInputEvent&) = 0;
  // Instrumentation from core/exported layer.
  virtual void DidCommitLoadForLocalFrame(LocalFrame*) = 0;
  virtual void DidStartProvisionalLoad(LocalFrame*) = 0;
  virtual bool ScreencastEnabled() = 0;
  virtual void LayerTreeViewChanged(WebLayerTreeView*) = 0;
  virtual void RootLayerCleared() = 0;
};

}  // namespace blink

#endif  // WebDevToolsAgentCore_h
