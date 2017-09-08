// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_UNHANDLED_TAP_SERVICE_IMPL_H_
#define COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_UNHANDLED_TAP_SERVICE_IMPL_H_

#include "base/macros.h"
#include "components/contextual_search/browser/contextual_search_api_handler.h"
// TODO(donnd): which of the many versions of this is best?
#include "third_party/WebKit/public/web/unhandled_tap_info.mojom.h"

namespace contextual_search {

class ContextualSearchUnhandledTapServiceImpl
    : public blink::mojom::UnhandledTapNotifierService {
 public:
  explicit ContextualSearchUnhandledTapServiceImpl(
      ContextualSearchApiHandler* contextual_search_api_handler);
  ~ContextualSearchUnhandledTapServiceImpl() override;

  // Mojo ContextualSearchUnhandledTapService implementation.
  void ShowUnhandledTapUIIfNeeded(blink::mojom::UnhandledTapInfoPtr unhandled_tap_info) override;

 private:
  // The actual handler for calls through the service.
  ContextualSearchApiHandler* contextual_search_api_handler_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSearchUnhandledTapServiceImpl);
};

// static
void CreateContextualSearchUnhandledTapService(
    ContextualSearchApiHandler* contextual_search_api_handler,
    blink::mojom::UnhandledTapNotifierServiceRequest request);

}  // namespace contextual_search

#endif  // COMPONENTS_CONTEXTUAL_SEARCH_BROWSER_CONTEXTUAL_SEARCH_UNHANDLED_TAP_SERVICE_IMPL_H_
