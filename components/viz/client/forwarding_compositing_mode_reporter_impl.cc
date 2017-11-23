// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/client/forwarding_compositing_mode_reporter_impl.h"

namespace viz {

ForwardingCompositingModeReporterImpl::ForwardingCompositingModeReporterImpl()
    : forwarding_source_binding_(this) {}

ForwardingCompositingModeReporterImpl::
    ~ForwardingCompositingModeReporterImpl() = default;

mojom::CompositingModeWatcherPtr
ForwardingCompositingModeReporterImpl::BindAsWatcher() {
  if (forwarding_source_binding_.is_bound())
    forwarding_source_binding_.Unbind();
  mojom::CompositingModeWatcherPtr ptr;
  forwarding_source_binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void ForwardingCompositingModeReporterImpl::AddCompositingModeWatcher(
    mojom::CompositingModeWatcherPtr watcher) {
  if (!gpu_)
    watcher->CompositingModeFallbackToSoftware();
  watchers_.AddPtr(std::move(watcher));
}

void ForwardingCompositingModeReporterImpl::
    CompositingModeFallbackToSoftware() {
  gpu_ = false;
  watchers_.ForAllPtrs([](mojom::CompositingModeWatcher* watcher) {
    watcher->CompositingModeFallbackToSoftware();
  });
}

}  // namespace viz
