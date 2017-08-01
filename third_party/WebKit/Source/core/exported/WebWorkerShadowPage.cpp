// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/exported/WebWorkerShadowPage.h"

#include "core/exported/WebFactory.h"
#include "core/exported/WebViewBase.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "public/platform/Platform.h"
#include "public/web/WebSettings.h"

namespace blink {

WebWorkerShadowPage::WebWorkerShadowPage(Client* client,
                                         bool data_saver_enabled)
    : web_view_(WebFactory::GetInstance().CreateWebViewBase(
          nullptr,
          kWebPageVisibilityStateVisible)),
      main_frame_(WebFactory::GetInstance()
                      .CreateMainWebLocalFrameBase(web_view_, this, nullptr)),
      client_(client) {
  DCHECK(IsMainThread());

  // Create 'shadow page', which is never displayed and is used mainly to
  // provide a context for loading on the main thread.
  //
  // TODO: This does mostly same as WebSharedWorkerImpl::initializeLoader.
  // This code, and probably most of the code in this class should be shared
  // with SharedWorker.
  WebSettings* settings = web_view_->GetSettings();

  // TODO: http://crbug.com/363843. This needs to find a better way to
  // not create graphics layers.
  settings->SetAcceleratedCompositingEnabled(false);

  settings->SetDataSaverEnabled(data_saver_enabled);
  main_frame_->SetDevToolsAgentClient(client_);

  service_manager::mojom::InterfaceProviderPtr provider;
  mojo::MakeRequest(&provider);
  interface_provider_.Bind(std::move(provider));
}

WebWorkerShadowPage::~WebWorkerShadowPage() {
  DCHECK(IsMainThread());
  // Detach the client before closing the view to avoid getting called back.
  main_frame_->SetClient(nullptr);
  web_view_->Close();
  main_frame_->Close();
}

void WebWorkerShadowPage::Load(const KURL& script_url) {
  DCHECK(IsMainThread());
  // Construct substitute data source for the 'shadow page'. We only need it
  // to have same origin as the worker so the loading checks work correctly.
  CString content("");
  RefPtr<SharedBuffer> buffer(
      SharedBuffer::Create(content.data(), content.length()));
  is_loading_ = true;
  main_frame_->GetFrame()->Loader().Load(
      FrameLoadRequest(0, ResourceRequest(script_url), SubstituteData(buffer)));
}

void WebWorkerShadowPage::SetContentSecurityPolicyAndReferrerPolicy(
    ContentSecurityPolicyResponseHeaders csp_headers,
    String referrer_policy) {
  DCHECK(IsMainThread());
  Document* document = main_frame_->GetFrame()->GetDocument();
  ContentSecurityPolicy* content_security_policy =
      ContentSecurityPolicy::Create();
  content_security_policy->SetOverrideURLForSelf(document->Url());
  content_security_policy->DidReceiveHeaders(csp_headers);
  document->InitContentSecurityPolicy(content_security_policy);
  if (!referrer_policy.IsNull())
    document->ParseAndSetReferrerPolicy(referrer_policy);
}

void WebWorkerShadowPage::DidFinishDocumentLoad() {
  DCHECK(IsMainThread());
  is_loading_ = false;
  client_->OnShadowPageLoaded();
}

std::unique_ptr<WebApplicationCacheHost>
WebWorkerShadowPage::CreateApplicationCacheHost(
    WebApplicationCacheHostClient* appcache_host_client) {
  DCHECK(IsMainThread());
  return client_->CreateApplicationCacheHost(appcache_host_client);
}

void WebWorkerShadowPage::FrameDetached(WebLocalFrame* frame, DetachType type) {
  DCHECK(IsMainThread());
  DCHECK(type == DetachType::kRemove && frame->Parent());
  DCHECK(frame->FrameWidget());
  frame->Close();
}

service_manager::InterfaceProvider*
WebWorkerShadowPage::GetInterfaceProvider() {
  DCHECK(IsMainThread());
  return &interface_provider_;
}

std::unique_ptr<blink::WebURLLoader> WebWorkerShadowPage::CreateURLLoader(
    const WebURLRequest& request,
    SingleThreadTaskRunner* task_runner) {
  DCHECK(IsMainThread());
  // TODO(yhirano): Stop using Platform::CreateURLLoader() here.
  return Platform::Current()->CreateURLLoader(request, task_runner);
}

}  // namespace blink
