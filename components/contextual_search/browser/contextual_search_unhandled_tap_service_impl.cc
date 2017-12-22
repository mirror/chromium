// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/contextual_search/browser/contextual_search_unhandled_tap_service_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "components/contextual_search/browser/contextual_search_js_api_handler.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace contextual_search {

ContextualSearchUnhandledTapServiceImpl::
    ContextualSearchUnhandledTapServiceImpl(
        content::RenderFrameHost* render_frame_host,
        contextual_search::ContextualSearchJsApiHandler* api_handler,
        blink::mojom::UnhandledTapNotifierRequest request)
    : render_frame_host_(render_frame_host),
      //   api_handler_(api_handler),
      binding_(this, std::move(request)) {
  DVLOG(0) << "ctxs constructing ContextualSearchUnhandledTapServiceImpl";
  DCHECK(render_frame_host_);
  //  DCHECK(api_handler_);

  /*
  binding_.set_connection_error_handler(
      base::BindOnce(&ContextualSearchUnhandledTapServiceImpl::
                     OnUnhandledTapServiceConnectionError,
                     this));
                     */
}

ContextualSearchUnhandledTapServiceImpl::
    ~ContextualSearchUnhandledTapServiceImpl() {}

void ContextualSearchUnhandledTapServiceImpl::ShowUnhandledTapUIIfNeeded(
    blink::mojom::UnhandledTapInfoPtr unhandled_tap_info) {
  DVLOG(0) << "ctxs "
              "ContextualSearchUnhandledTapServiceImpl::"
              "ShowUnhandledTapUIIfNeeded";
  // contextual_search_js_api_handler_->ShowUnhandledTapUIIfNeeded(
  //    std::move(unhandled_tap_info));
}

void ContextualSearchUnhandledTapServiceImpl::
    OnUnhandledTapServiceConnectionError() {
  DVLOG(0) << "ctxs ERROR OnUnhandledTapServiceConnectionError";
}

}  // namespace contextual_search
