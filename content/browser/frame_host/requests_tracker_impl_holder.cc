// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/requests_tracker_impl_holder.h"

#include "base/timer/timer.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

class RequestsTrackerImplHolder::Context final
    : public base::RefCounted<Context> {
 public:
  explicit Context(RenderProcessHost* process_host)
      : process_id_(process_host->GetID()) {
    DCHECK(!process_host->IsKeepAliveRefCountDisabled());
    process_host->IncrementKeepAliveRefCount();
  }

  void Detach() {
    if (detached_)
      return;
    detached_ = true;
    RenderProcessHost* process_host = RenderProcessHost::FromID(process_id_);
    if (!process_host || process_host->IsKeepAliveRefCountDisabled())
      return;

    process_host->DecrementKeepAliveRefCount();
  }

  void DetachLater() {
    timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(30), this,
                 &Context::Detach);
  }

 private:
  friend class base::RefCounted<Context>;
  ~Context() { Detach(); }

  base::OneShotTimer timer_;
  int process_id_;
  bool detached_ = false;

  DISALLOW_COPY_AND_ASSIGN(Context);
};

class RequestsTrackerImplHolder::RequestsTrackerImpl final
    : public mojom::RequestsTracker {
 public:
  explicit RequestsTrackerImpl(scoped_refptr<Context> context)
      : context_(std::move(context)) {}
  ~RequestsTrackerImpl() override {}

 private:
  scoped_refptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(RequestsTrackerImpl);
};

RequestsTrackerImplHolder::RequestsTrackerImplHolder(
    mojom::RequestsTrackerRequest request,
    RenderProcessHost* process_host) {
  if (process_host->IsKeepAliveRefCountDisabled())
    return;
  if (!base::FeatureList::IsEnabled(
          features::kKeepAliveRendererForKeepaliveRequests)) {
    return;
  }
  context_ = base::MakeRefCounted<Context>(process_host);
  mojom::RequestsTrackerPtrInfo info;
  mojo::MakeStrongBinding(base::MakeUnique<RequestsTrackerImpl>(context_),
                          std::move(request));
}

RequestsTrackerImplHolder::~RequestsTrackerImplHolder() {
  if (context_)
    context_->DetachLater();
}

}  // namespace content
