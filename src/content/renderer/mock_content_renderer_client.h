// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MOCK_CONTENT_RENDERER_CLIENT_H_
#define CONTENT_RENDERER_MOCK_CONTENT_RENDERER_CLIENT_H_
#pragma once

#include "base/compiler_specific.h"
#include "content/renderer/content_renderer_client.h"

namespace content {

// Base class for unit tests that need to mock the ContentRendererClient.
class MockContentRendererClient : public ContentRendererClient {
 public:
  virtual ~MockContentRendererClient();
  virtual void RenderThreadStarted() OVERRIDE;
  virtual void RenderViewCreated(RenderView* render_view) OVERRIDE;
  virtual void SetNumberOfViews(int number_of_views) OVERRIDE;
  virtual SkBitmap* GetSadPluginBitmap() OVERRIDE;
  virtual std::string GetDefaultEncoding() OVERRIDE;
  virtual WebKit::WebPlugin* CreatePlugin(
      RenderView* render_view,
      WebKit::WebFrame* frame,
      const WebKit::WebPluginParams& params) OVERRIDE;
  virtual void ShowErrorPage(RenderView* render_view,
                             WebKit::WebFrame* frame,
                             int http_status_code) OVERRIDE;
  virtual std::string GetNavigationErrorHtml(
      const WebKit::WebURLRequest& failed_request,
      const WebKit::WebURLError& error) OVERRIDE;
  virtual bool RunIdleHandlerWhenWidgetsHidden() OVERRIDE;
  virtual bool AllowPopup(const GURL& creator) OVERRIDE;
  virtual bool ShouldFork(WebKit::WebFrame* frame,
                          const GURL& url,
                          bool is_content_initiated,
                          bool* send_referrer) OVERRIDE;
  virtual bool WillSendRequest(WebKit::WebFrame* frame,
                               const GURL& url,
                               GURL* new_url) OVERRIDE;
  virtual bool ShouldPumpEventsDuringCookieMessage() OVERRIDE;
  virtual void DidCreateScriptContext(WebKit::WebFrame* frame) OVERRIDE;
  virtual void DidDestroyScriptContext(WebKit::WebFrame* frame) OVERRIDE;
  virtual void DidCreateIsolatedScriptContext(
      WebKit::WebFrame* frame) OVERRIDE;
  virtual unsigned long long VisitedLinkHash(const char* canonical_url,
                                             size_t length) OVERRIDE;
  virtual bool IsLinkVisited(unsigned long long link_hash) OVERRIDE;
  virtual void PrefetchHostName(const char* hostname, size_t length) OVERRIDE;
  virtual bool ShouldOverridePageVisibilityState(
      const RenderView* render_view,
      WebKit::WebPageVisibilityState* override_state) const OVERRIDE;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MOCK_CONTENT_RENDERER_CLIENT_H_
