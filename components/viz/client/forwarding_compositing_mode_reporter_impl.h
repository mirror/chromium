// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_CLIENT_FORWARDING_COMPOSITING_MODE_REPORTER_IMPL_H_
#define COMPONENTS_VIZ_CLIENT_FORWARDING_COMPOSITING_MODE_REPORTER_IMPL_H_

#include "components/viz/client/viz_client_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/viz/public/interfaces/compositing/compositing_mode_watcher.mojom.h"

namespace viz {

class VIZ_CLIENT_EXPORT ForwardingCompositingModeReporterImpl
    : public mojom::CompositingModeReporter,
      public mojom::CompositingModeWatcher {
 public:
  ForwardingCompositingModeReporterImpl();
  ~ForwardingCompositingModeReporterImpl() override;

  // Bind this object into a mojo pointer and returns it. The caller should
  // register the mojom pointer with another CompositingModeReporter. Then
  // that reporter's state will be forwarded to watchers of this reporter.
  mojom::CompositingModeWatcherPtr BindAsWatcher();

  // mojom::CompositingModeReporter implementation.
  void AddCompositingModeWatcher(
      mojom::CompositingModeWatcherPtr watcher) override;

  // mojom::CompositingModeWatcher implementation.
  void CompositingModeFallbackToSoftware() override;

 private:
  // A mirror of the state from the authoritative CompositingModeReporter that
  // this watches.
  bool gpu_ = true;
  mojo::Binding<mojom::CompositingModeWatcher> forwarding_source_binding_;
  mojo::InterfacePtrSet<mojom::CompositingModeWatcher> watchers_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_CLIENT_FORWARDING_COMPOSITING_MODE_REPORTER_IMPL_H_
