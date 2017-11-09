// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DEVTOOLS_DEVTOOLS_AGENT_H_
#define CONTENT_RENDERER_DEVTOOLS_DEVTOOLS_AGENT_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/containers/flat_map.h"
#include "content/common/content_export.h"
#include "content/common/devtools.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/WebKit/public/web/WebDevToolsAgentClient.h"

namespace blink {
class WebDevToolsAgent;
}

class GURL;

namespace content {

class RenderFrameImpl;
struct Manifest;
struct ManifestDebugInfo;

// Implementation of content.mojom.DevToolsAgent interface for RenderFrameImpl.
class CONTENT_EXPORT DevToolsAgent : public RenderFrameObserver,
                                     public blink::WebDevToolsAgentClient,
                                     public mojom::DevToolsAgent {
 public:
  explicit DevToolsAgent(RenderFrameImpl* frame);
  ~DevToolsAgent() override;

  static blink::WebDevToolsAgentClient::WebKitClientMessageLoop*
      createMessageLoopWrapper();

  void Bind(mojom::DevToolsAgentAssociatedRequest request);
  base::WeakPtr<DevToolsAgent> GetWeakPtr();
  bool IsAttached();
  void DetachAllSessions();
  void ContinueProgram();

 private:
  friend class DevToolsAgentTest;
  class SessionProxy;
  class MessageImpl;

  // RenderFrameObserver implementation.
  void WidgetWillClose() override;
  void OnDestruct() override;

  // mojom::DevToolsAgent implementation.
  void AttachDevToolsSession(
      mojom::DevToolsSessionHostPtr session_host,
      mojom::DevToolsSessionRequest session_request,
      const base::Optional<std::string>& reattach_state) override;
  void DetachSession(int session_id);

  // WebDevToolsAgentClient implementation.
  void SendProtocolMessage(int session_id,
                           int call_id,
                           const blink::WebString& message,
                           const blink::WebString& state_cookie) override;
  blink::WebDevToolsAgentClient::WebKitClientMessageLoop*
  CreateClientMessageLoop() override;
  void WillEnterDebugLoop() override;
  void DidExitDebugLoop() override;
  bool RequestDevToolsForFrame(int session_id, blink::WebLocalFrame* frame) override;
  void EnableTracing(const blink::WebString& category_filter) override;
  void DisableTracing() override;
  void SetCPUThrottlingRate(double rate) override;

  blink::WebDevToolsAgent* GetWebAgent();
  void DispatchOnInspectorBackend(int session_id,
                                  int call_id,
                                  const std::string& method,
                                  const std::string& message);
  void InspectElement(int session_id, const gfx::Point& point);
  void OnRequestNewWindowCompleted(int session_id, bool success);
  void GotManifest(int session_id,
                   int command_id,
                   const GURL& manifest_url,
                   const Manifest& manifest,
                   const ManifestDebugInfo& debug_info);
  void SendChunkedMessage(int session_id,
                          int call_id,
                          const std::string& message,
                          const std::string& post_state);

  mojo::AssociatedBinding<mojom::DevToolsAgent> binding_;
  int last_session_id_ = 0;
  base::flat_map<int, SessionProxy*> session_proxies_;
  base::flat_map<int, mojom::DevToolsSessionHostPtr> session_hosts_;
  bool paused_;
  RenderFrameImpl* frame_;
  base::Callback<void(int, int, const std::string&, const std::string&)>
      send_protocol_message_callback_for_test_;
  base::WeakPtrFactory<DevToolsAgent> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsAgent);
};

}  // namespace content

#endif  // CONTENT_RENDERER_DEVTOOLS_DEVTOOLS_AGENT_H_
