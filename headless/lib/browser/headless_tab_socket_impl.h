// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_TAB_SOCKET_IMPL_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_TAB_SOCKET_IMPL_H_

#include <list>

#include "base/synchronization/lock.h"
#include "headless/lib/headless_render_frame_controller.mojom.h"
#include "headless/lib/tab_socket.mojom.h"
#include "headless/public/headless_tab_socket.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace headless {

class HeadlessTabSocketImpl : public HeadlessTabSocket, public TabSocket {
 public:
  HeadlessTabSocketImpl();
  ~HeadlessTabSocketImpl() override;

  // HeadlessTabSocket implementation:
  void SendMessageToContext(const std::string& message,
                            int v8_execution_context_id) override;
  void SetListener(Listener* listener) override;

  // TabSocket implementation:
  void SendMessageToEmbedder(const std::string& message,
                             int32_t v8_execution_context_id) override;

  void CreateMojoService(mojo::InterfaceRequest<TabSocket> request);

  // Called by WebContents::ForEachFrame. If |render_frame_host| matches
  // |target_devtools_frame_id| it requests the RenderFrameController
  // installs TabSocket bindings in |world_id|, otherwise it does nothing.
  void MaybeInstallTabSocketBindings(
      std::string target_devtools_frame_id,
      int v8_execution_context_id,
      base::Callback<void(bool)> callback,
      content::RenderFrameHost* render_frame_host);

 private:
  base::Lock lock_;  // Protects everything below.
  using Message = std::pair<std::string, int>;
  using MessageQueue = std::list<Message>;

  MessageQueue from_tab_message_queue_;
  Listener* listener_;  // NOT OWNED

  mojo::BindingSet<TabSocket> mojo_bindings_;

  std::map<int, std::string> v8_execution_context_id_to_frame_id_;
  std::map<std::string, HeadlessRenderFrameControllerPtr>
      render_frame_controllers_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessTabSocketImpl);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_TAB_SOCKET_IMPL_H_
