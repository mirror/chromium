// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_CONNECTION_H_
#define PPAPI_PROXY_CONNECTION_H_

#include "ipc/ipc_message.h"
#include "ipc/ipc_sender.h"
#include "ppapi/proxy/plugin_dispatcher.h"

namespace ppapi {
namespace proxy {

class RendererSenderWrapper : public IPC::Sender {
 public:
  explicit RendererSenderWrapper(IPC::Sender* in_process_sender)
      : in_process_sender_(in_process_sender) {}
  explicit RendererSenderWrapper(
      base::WeakPtr<PluginDispatcher> out_of_process_sender)
      : in_process_sender_(nullptr),
        out_of_process_sender_(out_of_process_sender) {}
  ~RendererSenderWrapper() {}

  // IPC::Sender
  bool Send(IPC::Message* msg) override {
    if (in_process_sender_) {
      return in_process_sender_->Send(msg);
    } else {
      // Intentionally crash if the PluginDispatcher has been destroyed.
      // crbug.com/725033.
      CHECK(out_of_process_sender_);
      return out_of_process_sender_->Send(msg);
    }
  }

 private:
  // Will be nullptr if the Connection is an out-of-process connection.
  IPC::Sender* in_process_sender_;

  // Will be nullptr if the Connection is an in-process connection.
  base::WeakPtr<PluginDispatcher> out_of_process_sender_;
};

// This struct holds the channels that a resource uses to send message to the
// browser and renderer.
struct Connection {
  Connection() : browser_sender(0),
                 renderer_sender(0),
                 in_process(false),
                 browser_sender_routing_id(MSG_ROUTING_NONE) {
  }
  Connection(IPC::Sender* browser,
             base::WeakPtr<PluginDispatcher> out_of_process_renderer_sender)
      : browser_sender(browser),
        renderer_sender(out_of_process_renderer_sender),
        in_process(false),
        browser_sender_routing_id(MSG_ROUTING_NONE) {}
  Connection(IPC::Sender* browser,
             IPC::Sender* in_process_renderer_sender,
             int routing_id)
      : browser_sender(browser),
        renderer_sender(in_process_renderer_sender),
        in_process(true),
        browser_sender_routing_id(routing_id) {}

  IPC::Sender* browser_sender;
  RendererSenderWrapper renderer_sender;
  bool in_process;
  // We need to use a routing ID when a plugin is in-process, and messages are
  // sent back from the browser to the renderer. This is so that messages are
  // routed to the proper RenderFrameImpl.
  int browser_sender_routing_id;
};

}  // namespace proxy
}  // namespace ppapi


#endif  // PPAPI_PROXY_CONNECTION_H_

