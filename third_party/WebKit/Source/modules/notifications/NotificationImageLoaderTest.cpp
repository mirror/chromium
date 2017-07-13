// Copyright 2016 Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/notifications/NotificationImageLoader.h"

#include "core/dom/ExecutionContext.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/loader/fetch/MemoryCache.h"
#include "platform/testing/HistogramTester.h"
#include "platform/testing/TestingPlatformSupport.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/Functional.h"
#include "public/platform/Platform.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebURLLoaderMockFactory.h"
#include "public/platform/modules/notifications/WebNotificationConstants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {
namespace {

enum class LoadState { kNotLoaded, kLoadFailed, kLoadSuccessful };

constexpr char kBaseUrl[] = "http://test.com/";
constexpr char kBaseDir[] = "notifications/";
constexpr char kIcon500x500[] = "500x500.png";

// This mirrors the definition in NotificationImageLoader.cpp.
constexpr unsigned long kImageFetchTimeoutInMs = 90000;
static_assert(kImageFetchTimeoutInMs > 1000.0,
              "kImageFetchTimeoutInMs must be greater than 1000ms.");

constexpr char kIcon3000x1000[] = "3000x1000.png";
constexpr char kIcon3000x2000[] = "3000x2000.png";

// Version of the NotificationImageLoader that quits the run loop when loading
// and resizing of the image has completed.
class TestNotificationImageLoader : public NotificationImageLoader {
 public:
  explicit TestNotificationImageLoader(Type type)
      : NotificationImageLoader(type) {}

  ~TestNotificationImageLoader() override = default;

  // Spins a run loop until the signal that the image load has been completed
  // has been received.
  void WaitForImageLoadCompleteSignal() {
    DCHECK(!waiting_);
    waiting_ = true;

    testing::EnterRunLoop();
  }

  // Called when the image has been loaded, will exit the run loop if it's been
  // previously entered by WaitForImageLoadCompleteSignal().
  void SignalImageLoadCompleteForTesting() override {
    if (!waiting_)
      return;

    testing::ExitRunLoop();
    waiting_ = false;
  }

 private:
  bool waiting_ = false;
};

class NotificationImageLoaderTest : public ::testing::Test {
 public:
  NotificationImageLoaderTest() : page_(DummyPageHolder::Create()) {}

  ~NotificationImageLoaderTest() override {
    DCHECK(loader_);

    loader_->Stop();
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  void CreateLoaderWithType(NotificationImageLoader::Type type) {
    loader_ = new TestNotificationImageLoader(type);
  }

  // Registers a mocked URL. When fetched it will be loaded form the test data
  // directory.
  WebURL RegisterMockedURL(const String& file_name) {
    WebURL registered_url = URLTestHelpers::RegisterMockedURLLoadFromBase(
        kBaseUrl, testing::CoreTestDataPath(kBaseDir), file_name, "image/png");
    return registered_url;
  }

  // Callback for the NotificationImageLoader. This will set the state of the
  // load as either success or failed based on whether the bitmap is empty.
  void ImageLoaded(const SkBitmap& image) {
    image_ = image;

    if (!image.empty())
      loaded_ = LoadState::kLoadSuccessful;
    else
      loaded_ = LoadState::kLoadFailed;
  }

  void LoadImage(const KURL& url) {
    DCHECK(loader_);
    loader_->Start(
        Context(), url,
        Bind(&NotificationImageLoaderTest::ImageLoaded, WTF::Unretained(this)));
  }

  ExecutionContext* Context() const { return &page_->GetDocument(); }
  LoadState Loaded() const { return loaded_; }

 protected:
  HistogramTester histogram_tester_;
  std::unique_ptr<DummyPageHolder> page_;
  Persistent<TestNotificationImageLoader> loader_;
  LoadState loaded_ = LoadState::kNotLoaded;
  SkBitmap image_;
};

TEST_F(NotificationImageLoaderTest, SuccessTest) {
  CreateLoaderWithType(NotificationImageLoader::Type::kIcon);

  KURL url = RegisterMockedURL(kIcon500x500);
  LoadImage(url);
  histogram_tester_.ExpectTotalCount("Notifications.LoadFinishTime.Icon", 0);
  histogram_tester_.ExpectTotalCount("Notifications.LoadFileSize.Icon", 0);
  histogram_tester_.ExpectTotalCount("Notifications.LoadFailTime.Icon", 0);

  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  loader_->WaitForImageLoadCompleteSignal();

  EXPECT_EQ(LoadState::kLoadSuccessful, Loaded());
  histogram_tester_.ExpectTotalCount("Notifications.LoadFinishTime.Icon", 1);
  histogram_tester_.ExpectUniqueSample("Notifications.LoadFileSize.Icon", 7439,
                                       1);
  histogram_tester_.ExpectTotalCount("Notifications.LoadFailTime.Icon", 0);
}

TEST_F(NotificationImageLoaderTest, TimeoutTest) {
  CreateLoaderWithType(NotificationImageLoader::Type::kIcon);

  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  // To test for a timeout, this needs to override the clock in the platform.
  // Just creating the mock platform will do everything to set it up.
  KURL url = RegisterMockedURL(kIcon500x500);
  LoadImage(url);

  // Run the platform for kImageFetchTimeoutInMs-1 seconds. This should not
  // result in a timeout.
  platform->RunForPeriodSeconds(kImageFetchTimeoutInMs / 1000 - 1);
  EXPECT_EQ(LoadState::kNotLoaded, Loaded());
  histogram_tester_.ExpectTotalCount("Notifications.LoadFinishTime.Icon", 0);
  histogram_tester_.ExpectTotalCount("Notifications.LoadFileSize.Icon", 0);
  histogram_tester_.ExpectTotalCount("Notifications.LoadFailTime.Icon", 0);

  // Now advance time until a timeout should be expected.
  platform->RunForPeriodSeconds(2);

  // If the loader times out, it calls the callback and returns an empty bitmap.
  EXPECT_EQ(LoadState::kLoadFailed, Loaded());
  histogram_tester_.ExpectTotalCount("Notifications.LoadFinishTime.Icon", 0);
  histogram_tester_.ExpectTotalCount("Notifications.LoadFileSize.Icon", 0);
  // Should log a non-zero failure time.
  histogram_tester_.ExpectTotalCount("Notifications.LoadFailTime.Icon", 1);
  histogram_tester_.ExpectBucketCount("Notifications.LoadFailTime.Icon", 0, 0);
}

TEST_F(NotificationImageLoaderTest, BadgeIsScaledDown) {
  CreateLoaderWithType(NotificationImageLoader::Type::kBadge);

  KURL url = RegisterMockedURL(kIcon500x500);
  LoadImage(url);

  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  loader_->WaitForImageLoadCompleteSignal();

  ASSERT_EQ(LoadState::kLoadSuccessful, Loaded());

  ASSERT_FALSE(image_.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxBadgeSizePx, image_.width());
  ASSERT_EQ(kWebNotificationMaxBadgeSizePx, image_.height());
}

TEST_F(NotificationImageLoaderTest, IconIsScaledDown) {
  CreateLoaderWithType(NotificationImageLoader::Type::kIcon);

  KURL url = RegisterMockedURL(kIcon500x500);
  LoadImage(url);

  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  loader_->WaitForImageLoadCompleteSignal();

  ASSERT_EQ(LoadState::kLoadSuccessful, Loaded());

  ASSERT_FALSE(image_.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxIconSizePx, image_.width());
  ASSERT_EQ(kWebNotificationMaxIconSizePx, image_.height());
}

TEST_F(NotificationImageLoaderTest, ActionIconIsScaledDown) {
  CreateLoaderWithType(NotificationImageLoader::Type::kActionIcon);

  KURL url = RegisterMockedURL(kIcon500x500);
  LoadImage(url);

  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  loader_->WaitForImageLoadCompleteSignal();

  ASSERT_EQ(LoadState::kLoadSuccessful, Loaded());

  ASSERT_FALSE(image_.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxActionIconSizePx, image_.width());
  ASSERT_EQ(kWebNotificationMaxActionIconSizePx, image_.height());
}

TEST_F(NotificationImageLoaderTest, DownscalingPreserves3_1AspectRatio) {
  CreateLoaderWithType(NotificationImageLoader::Type::kImage);

  KURL url = RegisterMockedURL(kIcon3000x1000);
  LoadImage(url);

  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  loader_->WaitForImageLoadCompleteSignal();

  ASSERT_EQ(LoadState::kLoadSuccessful, Loaded());

  ASSERT_FALSE(image_.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxImageWidthPx, image_.width());
  ASSERT_EQ(kWebNotificationMaxImageWidthPx / 3, image_.height());
}

TEST_F(NotificationImageLoaderTest, DownscalingPreserves3_2AspectRatio) {
  CreateLoaderWithType(NotificationImageLoader::Type::kImage);

  KURL url = RegisterMockedURL(kIcon3000x2000);
  LoadImage(url);

  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  loader_->WaitForImageLoadCompleteSignal();

  ASSERT_EQ(LoadState::kLoadSuccessful, Loaded());

  ASSERT_FALSE(image_.drawsNothing());
  ASSERT_EQ(kWebNotificationMaxImageHeightPx * 3 / 2, image_.width());
  ASSERT_EQ(kWebNotificationMaxImageHeightPx, image_.height());
}

}  // namspace
}  // namespace blink
