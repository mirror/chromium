// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GUEST_VIEW_RENDERER_GUEST_VIEW_CONTAINER_H_
#define COMPONENTS_GUEST_VIEW_RENDERER_GUEST_VIEW_CONTAINER_H_

#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/renderer/browser_plugin_delegate.h"
#include "ipc/ipc_message.h"

namespace guest_view {

class GuestViewRequest;

class GuestViewContainer : public content::BrowserPluginDelegate {
 public:
  explicit GuestViewContainer(content::RenderFrame* render_frame);

  static GuestViewContainer* FromID(int element_instance_id);

  // IssueRequest queues up a |request| until the container is ready and
  // the browser process has responded to the last request if it's still
  // pending.
  void IssueRequest(linked_ptr<GuestViewRequest> request);

  int element_instance_id() const { return element_instance_id_; }
  content::RenderFrame* render_frame() const { return render_frame_; }

  // Called by GuestViewContainerDispatcher to dispatch message to this
  // container.
  bool OnMessageReceived(const IPC::Message& message);

  // Destroys this GuestViewContainer after performing necessary cleanup.
  void Destroy();

  // Called when the embedding RenderFrame is destroyed.
  virtual void OnRenderFrameDestroyed() {}

  // Called to respond to IPCs from the browser process that have not been
  // handled by GuestViewContainer.
  virtual bool OnMessage(const IPC::Message& message);

  // Called to perform actions when a GuestViewContainer gets a geometry.
  virtual void OnReady() {}

  // Called to perform actions when a GuestViewContainer is about to be
  // destroyed.
  // Note that this should be called exactly once.
  virtual void OnDestroy() {}

 protected:
  ~GuestViewContainer() override;

 private:
  class RenderFrameLifetimeObserver;
  friend class RenderFrameLifetimeObserver;

  void RenderFrameDestroyed();

  void EnqueueRequest(linked_ptr<GuestViewRequest> request);
  void PerformPendingRequest();
  void HandlePendingResponseCallback(const IPC::Message& message);

  void OnHandleCallback(const IPC::Message& message);

  // BrowserPluginDelegate implementation.
  void Ready() final;
  void SetElementInstanceID(int element_instance_id) final;
  void DidDestroyElement() final;

  int element_instance_id_;
  content::RenderFrame* render_frame_;
  scoped_ptr<RenderFrameLifetimeObserver> render_frame_lifetime_observer_;

  bool ready_;
  bool in_destruction_;

  std::deque<linked_ptr<GuestViewRequest> > pending_requests_;
  linked_ptr<GuestViewRequest> pending_response_;

  DISALLOW_COPY_AND_ASSIGN(GuestViewContainer);
};

}  // namespace guest_view

#endif  // COMPONENTS_GUEST_VIEW_RENDERER_GUEST_VIEW_CONTAINER_H_
