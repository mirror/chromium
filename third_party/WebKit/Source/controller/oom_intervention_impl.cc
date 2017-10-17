// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/Source/controller/oom_intervention_impl.h"

#include "core/frame/LocalFrame.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/WebKit/Source/core/page/ScopedPageSuspender.h"

namespace blink {

// static
void OomInterventionImpl::Create(LocalFrame* frame,
                                 mojom::OomInterventionRequest request) {
  mojo::MakeStrongBinding(
      std::unique_ptr<OomInterventionImpl>(new OomInterventionImpl(frame)),
      std::move(request));
}

OomInterventionImpl::OomInterventionImpl(LocalFrame* frame) {
  LOG(ERROR) << "Created";
}

OomInterventionImpl::~OomInterventionImpl() {
  LOG(ERROR) << "Deleted";
}

void OomInterventionImpl::OnNearOomDetected() {
  LOG(ERROR) << "OnNearOomDetected";
  if (!suspender_) {
    LOG(ERROR) << "Pausing the page";
    suspender_.reset(new ScopedPageSuspender());
  }
}

void OomInterventionImpl::OnInterventionDeclined() {
  LOG(ERROR) << "OnInterventionDeclined";
  suspender_.reset();
}

}  // namespace blink
