// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebWorkerShadowPage_h
#define WebWorkerShadowPage_h

#include "core/frame/WebLocalFrameBase.h"
#include "platform/network/ContentSecurityPolicyResponseHeaders.h"
#include "public/web/WebDevToolsAgentClient.h"
#include "public/web/WebFrameClient.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

class WebServiceWorkerNetworkProvider;
class WebView;

class WebWorkerShadowPage : public WebFrameClient {
 public:
  class Client : NON_EXPORTED_BASE(public WebDevToolsAgentClient) {
   public:
    virtual void OnShadowPageLoaded() = 0;
  };

  WebWorkerShadowPage(Client*, bool data_saver_enabled);
  ~WebWorkerShadowPage();

  void Load(const KURL& script_url);
  void SetContentSecurityPolicyAndReferrerPolicy(
      ContentSecurityPolicyResponseHeaders csp_headers,
      String referrer_policy);

  // WebFrameClient overrides.
  void FrameDetached(WebLocalFrame*, DetachType) override;
  void DidFinishDocumentLoad() override;
  service_manager::InterfaceProvider* GetInterfaceProvider() override;
  std::unique_ptr<blink::WebURLLoader> CreateURLLoader(
      const WebURLRequest&,
      SingleThreadTaskRunner*) override;

  void SetServiceWorkerNetworkProvider(
      std::unique_ptr<WebServiceWorkerNetworkProvider>);

  Document* GetDocument() { return main_frame_->GetFrame()->GetDocument(); }
  WebDevToolsAgent* DevToolsAgent() { return main_frame_->DevToolsAgent(); }

  bool IsLoading() const { return is_loading_; };

 private:
  // 'shadow page' - created to proxy loading requests from the worker.
  // WebView and WebFrame objects are close()'ed (where they're
  // deref'ed) when this EmbeddedWorkerImpl is destructed, therefore they
  // are guaranteed to exist while this object is around.
  WebView* web_view_;

  Persistent<WebLocalFrameBase> main_frame_;

  Client* client_;

  service_manager::InterfaceProvider interface_provider_;

  bool is_loading_ = false;
};

}  // namespace blink

#endif  // WebEmbeddedWorkerImpl_h
