// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/protocol/page_handler.h"

#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"

PageHandler::PageHandler(protocol::UberDispatcher* dispatcher)
    : frontend_(new protocol::Page::Frontend(dispatcher->channel())) {
  protocol::Page::Dispatcher::wire(dispatcher, this);
}

PageHandler::~PageHandler() = default;

// static
void PageHandler::ToggleAdBlocking(bool enabled,
                                   content::WebContents* web_contents) {
  if (!web_contents)
    return;
  if (auto* client =
          ChromeSubresourceFilterClient::FromWebContents(web_contents)) {
    client->ToggleForceActivationInCurrentWebContents(enabled);
  }
}

protocol::Response PageHandler::Enable() {
  enabled_ = true;
  return protocol::Response::FallThrough();
}

protocol::Response PageHandler::Disable() {
  enabled_ = false;
  ToggleAdBlocking(false /* enable */, web_contents_);
  return protocol::Response::FallThrough();
}

protocol::Response PageHandler::SetAdBlockingEnabled(bool enabled) {
  if (!enabled_)
    return protocol::Response::Error("Page domain is disabled.");
  ToggleAdBlocking(enabled, web_contents_);
  return protocol::Response::OK();
}
