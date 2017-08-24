// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DEVTOOLS_DEVTOOLS_AGENT_OBSERVER_H_
#define CONTENT_RENDERER_DEVTOOLS_DEVTOOLS_AGENT_OBSERVER_H_

namespace content {

class DevToolsAgentObserver {
 public:
  virtual void OnDevToolsAttachmentChanged() = 0;
};

}  // namespace content

#endif  // CONTENT_RENDERER_DEVTOOLS_DEVTOOLS_AGENT_OBSERVER_H_
