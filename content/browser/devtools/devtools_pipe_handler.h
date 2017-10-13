// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_PIPE_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_PIPE_HANDLER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/devtools_agent_host_client.h"

namespace base {
class Thread;
}

namespace content {

class PipeWrapper;

class DevToolsPipeHandler : public DevToolsAgentHostClient {
 public:
  explicit DevToolsPipeHandler(int handle);
  ~DevToolsPipeHandler() override;

  void StartServerOnHandlerThread(base::Thread* thread, int handle);

  void HandleMessage(const std::string& message);
  void DetachFromTarget();

  // DevToolsAgentHostClient overrides
  void DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                               const std::string& message) override;
  void AgentHostClosed(DevToolsAgentHost* agent_host,
                       bool replaced_with_another_client) override;

 private:
  std::unique_ptr<PipeWrapper> pipe_wrapper_;
  std::unique_ptr<base::Thread> thread_;
  scoped_refptr<DevToolsAgentHost> browser_target_;
  base::WeakPtrFactory<DevToolsPipeHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsPipeHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_PIPE_HANDLER_H_
