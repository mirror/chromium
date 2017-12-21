// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/LocalFrame.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class LocalFrameTest : public ::testing::Test {
 public:
  bool ShouldUseClientLoFiForRequest(
      const ResourceRequest& request,
      WebURLRequest::PreviewsState frame_previews_state) {
    return frame->ShouldUseClientLoFiForRequest(request, frame_previews_state);
  }

  Persistent<LocalFrame> frame;
};

TEST_F(LocalFrameTest, ShouldUseClientLoFiForRequest) {
  // Verify request value used if specified.
  ResourceRequest request;
  request.SetPreviewsState(WebURLRequest::kClientLoFiOn);
  EXPECT_TRUE(
      ShouldUseClientLoFiForRequest(request, WebURLRequest::kPreviewsOff));
  request.SetPreviewsState(WebURLRequest::kPreviewsOff);
  EXPECT_FALSE(
      ShouldUseClientLoFiForRequest(request, WebURLRequest::kClientLoFiOn));

  // Verify frame value used if previews state not specified on request.
  request.SetPreviewsState(WebURLRequest::kPreviewsUnspecified);
  EXPECT_TRUE(
      ShouldUseClientLoFiForRequest(request, WebURLRequest::kClientLoFiOn));
  EXPECT_FALSE(
      ShouldUseClientLoFiForRequest(request, WebURLRequest::kServerLitePageOn));

  // Verify conditional on scheme if ServerLoFiOn set.
  request.SetURL(KURL("https://secure.com"));
  EXPECT_TRUE(ShouldUseClientLoFiForRequest(
      request, WebURLRequest::kServerLoFiOn | WebURLRequest::kClientLoFiOn));
  request.SetURL(KURL("http://insecure.com"));
  EXPECT_FALSE(ShouldUseClientLoFiForRequest(
      request, WebURLRequest::kServerLoFiOn | WebURLRequest::kClientLoFiOn));
}

}  // namespace blink
