// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_UNHANDLED_TAP_SERVICE_IMPL_H_
#define COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_UNHANDLED_TAP_SERVICE_IMPL_H_

#include "base/macros.h"
#include "components/contextual_search/browser/contextual_search_js_api_handler.h"
// TODO(donnd): which of the many versions of this is best?
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/WebKit/public/platform/unhandled_tap_notifier.mojom.h"

namespace contextual_search {

class ContextualSearchUnhandledTapServiceImpl
    : public blink::mojom::UnhandledTapNotifier {
 public:
  ContextualSearchUnhandledTapServiceImpl(
      content::RenderFrameHost* render_frame_host,
      contextual_search::ContextualSearchJsApiHandler* api_handler,
      blink::mojom::UnhandledTapNotifierRequest request);

  ~ContextualSearchUnhandledTapServiceImpl() override;

  // Mojo ContextualSearchUnhandledTapService implementation.
  void ShowUnhandledTapUIIfNeeded(
      blink::mojom::UnhandledTapInfoPtr unhandled_tap_info) override;

 private:
  // The actual handler for calls through the service.
  //  ContextualSearchJsApiHandler* contextual_search_js_api_handler_;
  void OnUnhandledTapServiceConnectionError();

  content::RenderFrameHost* const render_frame_host_;

  //  contextual_search::ContextualSearchJsApiHandler* api_handler_;

  mojo::Binding<blink::mojom::UnhandledTapNotifier> binding_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSearchUnhandledTapServiceImpl);
};

}  // namespace contextual_search

#endif  // COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_UNHANDLED_TAP_SERVICE_IMPL_H_
