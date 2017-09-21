// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/chrome_devtools_session.h"

#include "chrome/browser/devtools/protocol/page_handler.h"
#include "content/public/browser/devtools_agent_host.h"

ChromeDevToolsSession::ChromeDevToolsSession(
    content::DevToolsAgentHost* agent_host)
    : dispatcher_(std::make_unique<protocol::UberDispatcher>(nullptr)) {
  dispatcher_->setFallThroughForNotFound(true);
  if (agent_host->GetWebContents()) {
    page_handler_ = std::make_unique<PageHandler>(agent_host->GetWebContents(),
                                                  dispatcher_.get());
  }
}

ChromeDevToolsSession::~ChromeDevToolsSession() = default;
