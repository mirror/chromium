// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebDevToolsAgentBase_h
#define WebDevToolsAgentBase_h

#include "core/inspector/InspectorEmulationAgent.h"
#include "core/inspector/InspectorPageAgent.h"
#include "core/inspector/InspectorSession.h"
#include "core/inspector/InspectorTracingAgent.h"
#include "public/platform/WebThread.h"
#include "public/web/WebDevToolsAgent.h"

namespace blink {

class LocalFrame;

// WebDevToolsAgentBase is a temporary class the provides a layer of abstraction
// for WebDevToolsAgentImpl. Mehtods that are declared public in
// WebDevToolsAgentImpl that are not overrides will be declared pure virtual in
// WebDevToolsAgentBase. Classes that then have a dependency on
// WebDevToolsAgentImpl will then take a dependency on WebDevToolsAgentBase
// instead, so we can remove cyclic dependencies in web/ and move classes from
// web/ into core/ or modules.
// TODO(sashab): Remove this class once WebDevToolsAgentImpl is in core/.
class CORE_EXPORT WebDevToolsAgentBase
    : public GarbageCollectedFinalized<WebDevToolsAgentBase>,
      NON_EXPORTED_BASE(public WebDevToolsAgent),
      public InspectorEmulationAgent::Client,
      public InspectorTracingAgent::Client,
      public InspectorPageAgent::Client,
      public InspectorSession::Client,
      protected WebThread::TaskObserver {
 public:
  DEFINE_INLINE_VIRTUAL_TRACE() {}

  virtual void PaintOverlay() = 0;
  virtual bool HandleInputEvent(const WebInputEvent&) = 0;

  // Instrumentation from web/ layer.
  virtual void DidCommitLoadForLocalFrame(LocalFrame*) = 0;
  virtual void DidStartProvisionalLoad(LocalFrame*) = 0;
  // virtual bool ScreencastEnabled() = 0;
  virtual void WillAddPageOverlay(const GraphicsLayer*) = 0;
  virtual void DidRemovePageOverlay(const GraphicsLayer*) = 0;
  virtual void LayerTreeViewChanged(WebLayerTreeView*) = 0;
  // virtual void RootLayerCleared() = 0;
};

}  // namespace blink

#endif  // WebDevToolsAgentBase_h
