// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "controller/oom_intervention_impl.h"

#include "core/page/ScopedPagePauser.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace blink {

// static
void OomInterventionImpl::RegisterInterface(InterfaceRegistry* registry) {
  DCHECK(registry);
  registry->AddInterface(WTF::Bind(&OomInterventionImpl::Create));
}

// static
void OomInterventionImpl::Create(mojom::OomInterventionRequest request) {
  mojo::MakeStrongBinding(std::make_unique<OomInterventionImpl>(),
                          std::move(request));
}

OomInterventionImpl::OomInterventionImpl() {
  LOG(ERROR) << "Created";
}

OomInterventionImpl::~OomInterventionImpl() = default;

void OomInterventionImpl::OnNearOomDetected() {
  LOG(ERROR) << "OnNearOomDetected";
  if (!pauser_) {
    LOG(ERROR) << "Pausing the page";
    pauser_.reset(new ScopedPagePauser());
  }
}

void OomInterventionImpl::OnInterventionDeclined() {
  LOG(ERROR) << "OnInterventionDeclined";
  pauser_.reset();
}

}  // namespace blink
