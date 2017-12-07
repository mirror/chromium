// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/content/renderer/content_previews_render_frame_observer.h"

#include "content/public/common/previews_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"

namespace data_reduction_proxy {

TEST(ContentPreviewsRenderFrameObserverTest,
     ValidatePreviewsStateWithResponseNoHeaders) {
  blink::WebURLResponse response_no_headers;

  EXPECT_TRUE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::PREVIEWS_UNSPECIFIED, response_no_headers));
  EXPECT_TRUE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::NOSCRIPT_ON, response_no_headers));
  EXPECT_TRUE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::CLIENT_LOFI_ON, response_no_headers));

  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::SERVER_LOFI_ON, response_no_headers));
  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::SERVER_LITE_PAGE_ON, response_no_headers));
}

TEST(ContentPreviewsRenderFrameObserverTest,
     ValidatePreviewsStateWithResponseLitePageHeader) {
  blink::WebURLResponse response_with_lite_page;
  response_with_lite_page.AddHTTPHeaderField("chrome-proxy-content-transform",
                                             "lite-page");

  EXPECT_TRUE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::SERVER_LITE_PAGE_ON, response_with_lite_page));

  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::PREVIEWS_UNSPECIFIED, response_with_lite_page));
  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::NOSCRIPT_ON, response_with_lite_page));
  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::CLIENT_LOFI_ON, response_with_lite_page));

  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::SERVER_LOFI_ON, response_with_lite_page));
}

TEST(ContentPreviewsRenderFrameObserverTest,
     ValidatePreviewsStateWithResponsePagePolicyHeader) {
  blink::WebURLResponse response_with_page_policy;
  response_with_page_policy.AddHTTPHeaderField("Chrome-Proxy",
                                               "Page-Policies=Empty-Image");

  EXPECT_TRUE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::SERVER_LOFI_ON, response_with_page_policy));

  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::PREVIEWS_UNSPECIFIED, response_with_page_policy));
  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::NOSCRIPT_ON, response_with_page_policy));
  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::CLIENT_LOFI_ON, response_with_page_policy));
  EXPECT_FALSE(
      ContentPreviewsRenderFrameObserver::ValidatePreviewsStateWithResponse(
          content::SERVER_LITE_PAGE_ON, response_with_page_policy));
}

}  // namespace data_reduction_proxy
