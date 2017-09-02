// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/contextual_search/browser/contextual_search_unhandled_tap_service_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "components/contextual_search/browser/contextual_search_api_handler.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace contextual_search {

ContextualSearchUnhandledTapServiceImpl::ContextualSearchUnhandledTapServiceImpl(
    ContextualSearchApiHandler* contextual_search_api_handler)
    : contextual_search_api_handler_(contextual_search_api_handler) {
  DVLOG(0) << "ctxs constructing ContextualSearchUnhandledTapServiceImpl";
}

ContextualSearchUnhandledTapServiceImpl::~ContextualSearchUnhandledTapServiceImpl() {}

void ContextualSearchUnhandledTapServiceImpl::ShowUnhandledTapUIIfNeeded(
    blink::mojom::UnhandledTapInfoPtr unhandled_tap_info) {
  DVLOG(0) << "ctxs ContextualSearchUnhandledTapServiceImpl::ShowUnhandledTapUIIfNeeded";
  contextual_search_api_handler_->ShowUnhandledTapUIIfNeeded();
}

// static
void CreateContextualSearchUnhandledTapService(
    ContextualSearchApiHandler* contextual_search_api_handler,
    blink::mojom::UnhandledTapNotifierServiceRequest request) {
  DVLOG(0) << "ctxs CreateContextualSearchUnhandledTapService";
  mojo::MakeStrongBinding(base::MakeUnique<ContextualSearchUnhandledTapServiceImpl>(
      contextual_search_api_handler),
                          std::move(request));
}

}  // namespace contextual_search
