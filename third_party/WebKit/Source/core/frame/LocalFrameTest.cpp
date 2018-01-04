// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/LocalFrame.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/Source/core/loader/EmptyClients.h"
#include "third_party/WebKit/Source/core/testing/DummyPageHolder.h"
#include "third_party/WebKit/Source/platform/loader/fetch/FetchParameters.h"

namespace blink {

class TestLocalFrameClient : public EmptyLocalFrameClient {
 public:
  explicit TestLocalFrameClient(
      WebURLRequest::PreviewsState frame_previews_state)
      : frame_previews_state_(frame_previews_state) {}
  ~TestLocalFrameClient() override = default;

  WebURLRequest::PreviewsState GetPreviewsStateForFrame() const override {
    return frame_previews_state_;
  }

 private:
  const WebURLRequest::PreviewsState frame_previews_state_;
};

class LocalFrameTest : public ::testing::Test {};

TEST_F(LocalFrameTest, MaybeAllowPlaceholderImageUsesSpecifiedRequestValue) {
  ResourceRequest request1;
  request1.SetURL(KURL("http://insecure.com"));
  request1.SetPreviewsState(WebURLRequest::kClientLoFiOn);
  FetchParameters params1(request1);
  DummyPageHolder::Create(IntSize(800, 600), nullptr,
                          new TestLocalFrameClient(WebURLRequest::kPreviewsOff))
      ->GetFrame()
      .MaybeAllowImagePlaceholder(params1);
  EXPECT_EQ(FetchParameters::kAllowPlaceholder,
            params1.GetPlaceholderImageRequestType());

  ResourceRequest request2;
  request2.SetURL(KURL("https://secure.com"));
  request2.SetPreviewsState(WebURLRequest::kPreviewsOff);
  FetchParameters params2(request2);
  DummyPageHolder::Create(
      IntSize(800, 600), nullptr,
      new TestLocalFrameClient(WebURLRequest::kClientLoFiOn))
      ->GetFrame()
      .MaybeAllowImagePlaceholder(params2);
  EXPECT_NE(FetchParameters::kAllowPlaceholder,
            params2.GetPlaceholderImageRequestType());
}

TEST_F(LocalFrameTest, MaybeAllowPlaceholderImageUsesFramePreviewsState) {
  ResourceRequest request1;
  request1.SetURL(KURL("http://insecure.com"));
  request1.SetPreviewsState(WebURLRequest::kPreviewsUnspecified);
  FetchParameters params1(request1);
  DummyPageHolder::Create(
      IntSize(800, 600), nullptr,
      new TestLocalFrameClient(WebURLRequest::kClientLoFiOn))
      ->GetFrame()
      .MaybeAllowImagePlaceholder(params1);
  EXPECT_EQ(FetchParameters::kAllowPlaceholder,
            params1.GetPlaceholderImageRequestType());

  ResourceRequest request2;
  request2.SetURL(KURL("http://insecure.com"));
  request2.SetPreviewsState(WebURLRequest::kPreviewsUnspecified);
  FetchParameters params2(request2);
  DummyPageHolder::Create(
      IntSize(800, 600), nullptr,
      new TestLocalFrameClient(WebURLRequest::kServerLitePageOn))
      ->GetFrame()
      .MaybeAllowImagePlaceholder(params2);
  EXPECT_NE(FetchParameters::kAllowPlaceholder,
            params2.GetPlaceholderImageRequestType());
}

TEST_F(LocalFrameTest,
       MaybeAllowPlaceholderImageConditionalOnSchemeForServerLoFi) {
  ResourceRequest request1;
  request1.SetURL(KURL("https://secure.com"));
  request1.SetPreviewsState(WebURLRequest::kPreviewsUnspecified);
  FetchParameters params1(request1);
  DummyPageHolder::Create(
      IntSize(800, 600), nullptr,
      new TestLocalFrameClient(WebURLRequest::kServerLoFiOn |
                               WebURLRequest::kClientLoFiOn))
      ->GetFrame()
      .MaybeAllowImagePlaceholder(params1);
  EXPECT_EQ(FetchParameters::kAllowPlaceholder,
            params1.GetPlaceholderImageRequestType());

  ResourceRequest request2;
  request2.SetURL(KURL("http://insecure.com"));
  request2.SetPreviewsState(WebURLRequest::kPreviewsUnspecified);
  FetchParameters params2(request2);
  DummyPageHolder::Create(
      IntSize(800, 600), nullptr,
      new TestLocalFrameClient(WebURLRequest::kServerLoFiOn |
                               WebURLRequest::kClientLoFiOn))
      ->GetFrame()
      .MaybeAllowImagePlaceholder(params2);
  EXPECT_NE(FetchParameters::kAllowPlaceholder,
            params2.GetPlaceholderImageRequestType());
}

}  // namespace blink
