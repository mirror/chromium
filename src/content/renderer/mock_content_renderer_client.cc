// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/mock_content_renderer_client.h"

#include <string>

namespace content {

MockContentRendererClient::~MockContentRendererClient() {
}

void MockContentRendererClient::RenderThreadStarted() {
}

void MockContentRendererClient::RenderViewCreated(RenderView* render_view) {
}

void MockContentRendererClient::SetNumberOfViews(int number_of_views) {
}

SkBitmap* MockContentRendererClient::GetSadPluginBitmap() {
  return NULL;
}

std::string MockContentRendererClient::GetDefaultEncoding() {
  return std::string();
}

WebKit::WebPlugin* MockContentRendererClient::CreatePlugin(
    RenderView* render_view,
    WebKit::WebFrame* frame,
    const WebKit::WebPluginParams& params) {
  return NULL;
}

void MockContentRendererClient::ShowErrorPage(RenderView* render_view,
                                              WebKit::WebFrame* frame,
                                              int http_status_code) {
}

std::string MockContentRendererClient::GetNavigationErrorHtml(
    const WebKit::WebURLRequest& failed_request,
    const WebKit::WebURLError& error) {
  return std::string();
}

bool MockContentRendererClient::RunIdleHandlerWhenWidgetsHidden() {
  return true;
}

bool MockContentRendererClient::AllowPopup(const GURL& creator) {
  return false;
}

bool MockContentRendererClient::ShouldFork(WebKit::WebFrame* frame,
                                           const GURL& url,
                                           bool is_content_initiated,
                                           bool* send_referrer) {
  return false;
}

bool MockContentRendererClient::WillSendRequest(WebKit::WebFrame* frame,
                                                const GURL& url,
                                                GURL* new_url) {
  return false;
}

bool MockContentRendererClient::ShouldPumpEventsDuringCookieMessage() {
  return false;
}

void MockContentRendererClient::DidCreateScriptContext(
    WebKit::WebFrame* frame) {
}

void MockContentRendererClient::DidDestroyScriptContext(
    WebKit::WebFrame* frame) {
}

void MockContentRendererClient::DidCreateIsolatedScriptContext(
    WebKit::WebFrame* frame) {
}

unsigned long long MockContentRendererClient::VisitedLinkHash(
    const char* canonical_url, size_t length) {
  return 0LL;
}

bool MockContentRendererClient::IsLinkVisited(unsigned long long link_hash) {
  return false;
}

void MockContentRendererClient::PrefetchHostName(
    const char* hostname, size_t length) {
}

bool MockContentRendererClient::ShouldOverridePageVisibilityState(
    const RenderView* render_view,
    WebKit::WebPageVisibilityState* override_state) const {
  return false;
}

}  // namespace content
