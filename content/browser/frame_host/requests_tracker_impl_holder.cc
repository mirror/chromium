// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/requests_tracker_impl_holder.h"

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"

namespace content {

namespace {
const int kDefaultTimeoutInSec = 30;
}

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
    auto timeout = base::TimeDelta::FromSeconds(kDefaultTimeoutInSec);
    int timeout_set_by_finch_in_sec = base::GetFieldTrialParamByFeatureAsInt(
        features::kKeepAliveRendererForKeepaliveRequests, "timeout_in_sec", 0);
    // Adopt only "reasonable" values.
    if (timeout_set_by_finch_in_sec > 0 && timeout_set_by_finch_in_sec <= 60)
      timeout = base::TimeDelta::FromSeconds(timeout_set_by_finch_in_sec);
    timer_.Start(FROM_HERE, timeout, this, &Context::Detach);
  }

  void AddBinding(std::unique_ptr<mojom::RequestsTracker> impl,
                  mojom::RequestsTrackerRequest request) {
    binding_set_.AddBinding(std::move(impl), std::move(request));
  }

 private:
  friend class base::RefCounted<Context>;
  ~Context() { Detach(); }

  base::OneShotTimer timer_;
  mojo::StrongBindingSet<mojom::RequestsTracker> binding_set_;
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
    RenderProcessHost* process_host) {
  if (process_host->IsKeepAliveRefCountDisabled())
    return;
  if (!base::FeatureList::IsEnabled(
          features::kKeepAliveRendererForKeepaliveRequests)) {
    return;
  }
  context_ = base::MakeRefCounted<Context>(process_host);
}

RequestsTrackerImplHolder::~RequestsTrackerImplHolder() {
  if (context_)
    context_->DetachLater();
}

void RequestsTrackerImplHolder::AddBinding(
    mojom::RequestsTrackerRequest request) {
  if (!context_)
    return;

  context_->AddBinding(base::MakeUnique<RequestsTrackerImpl>(context_),
                       std::move(request));
}

}  // namespace content
