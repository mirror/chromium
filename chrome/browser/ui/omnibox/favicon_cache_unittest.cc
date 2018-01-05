// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/omnibox/favicon_cache.h"

#include "components/favicon/core/test/mock_favicon_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/favicon_size.h"

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SaveArg;

namespace {

favicon_base::FaviconImageResult GetDummyFaviconResult() {
  favicon_base::FaviconImageResult result;

  result.icon_url = GURL("http://example.com/favicon.ico");

  SkBitmap bitmap;
  bitmap.allocN32Pixels(gfx::kFaviconSize, gfx::kFaviconSize);
  bitmap.eraseColor(SK_ColorBLUE);
  result.image = gfx::Image::CreateFrom1xBitmap(bitmap);

  return result;
}

void VerifyFetchedFaviconAndIncrementCount(int* count,
                                           const gfx::Image& favicon) {
  EXPECT_FALSE(favicon.IsEmpty());
  if (count)
    ++*count;
}

void ExpectNoAsyncFavicon(const gfx::Image& favicon) {
  FAIL() << "The favicon should have been provided synchronously by the cache, "
            "and this asynchronous callback should never have been called.";
}

}  // namespace

TEST(FaviconCacheTest, Basic) {
  GURL page_url("http://a.com/");
  favicon_base::FaviconImageCallback service_response_callback;

  testing::NiceMock<favicon::MockFaviconService> favicon_service;
  EXPECT_CALL(favicon_service, GetFaviconImageForPageURL(page_url, _, _))
      .WillOnce(DoAll(SaveArg<1>(&service_response_callback),
                      Return(base::CancelableTaskTracker::kBadTaskId)));

  FaviconCache cache(&favicon_service);
  gfx::Image result = cache.GetFaviconForPageUrl(
      page_url,
      base::BindOnce(&VerifyFetchedFaviconAndIncrementCount, nullptr));

  // Expect the synchronous result to be empty.
  EXPECT_TRUE(result.IsEmpty());

  // Simulate the favicon service returning the result.
  service_response_callback.Run(GetDummyFaviconResult());

  // Re-request the same favicon and expect a non-empty result now that the
  // cache is populated. The above EXPECT_CALL will also verify that the
  // backing FaviconService is not hit again.
  result = cache.GetFaviconForPageUrl(page_url,
                                      base::BindOnce(&ExpectNoAsyncFavicon));
  EXPECT_FALSE(result.IsEmpty());
}

TEST(FaviconCacheTest, MultipleRequestsAreCoalesced) {
  GURL page_url("http://a.com/");
  favicon_base::FaviconImageCallback service_response_callback;

  // Mock verifies that backing FaviconService is only called once.
  testing::NiceMock<favicon::MockFaviconService> favicon_service;
  EXPECT_CALL(favicon_service, GetFaviconImageForPageURL(page_url, _, _))
      .WillOnce(DoAll(SaveArg<1>(&service_response_callback),
                      Return(base::CancelableTaskTracker::kBadTaskId)));

  int callback_count = 0;

  FaviconCache cache(&favicon_service);
  for (int i = 0; i < 10; ++i) {
    cache.GetFaviconForPageUrl(
        page_url, base::BindOnce(&VerifyFetchedFaviconAndIncrementCount,
                                 &callback_count));
  }

  // Simulate the favicon service returning the result.
  service_response_callback.Run(GetDummyFaviconResult());

  EXPECT_EQ(10, callback_count);
}
