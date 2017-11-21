// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_preloaded_list.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

const base::FilePath kTestDataPath = base::FilePath(
    FILE_PATH_LITERAL("chrome/test/data/media/engagement/preload"));

// This sample data is auto generated at build time.
const base::FilePath::CharType kSampleDataPath[] = FILE_PATH_LITERAL(
    "gen/chrome/test/data/media/engagement/preload/"
    "media_engagement_preload_test.output");

const base::FilePath kMissingFilePath =
    kTestDataPath.AppendASCII("missing.output");

const base::FilePath kBadFormatFilePath =
    kTestDataPath.AppendASCII("bad_format.output");

const base::FilePath kBadContentsFilePath =
    kTestDataPath.AppendASCII("bad_contents.output");

}  // namespace

class MediaEngagementPreloadedListTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    preloaded_list_ = std::make_unique<MediaEngagementPreloadedList>();
  }

  bool LoadFromFile(base::FilePath path) {
    return preloaded_list_->LoadFromFile(path);
  }

  bool CheckOriginIsPresent(GURL url) {
    return preloaded_list_->CheckOriginIsPresent(url::Origin::Create(url));
  }

  base::FilePath GetFilePathRelativeToModule(
      base::FilePath::StringPieceType path) {
    base::FilePath module_dir;
#if defined(OS_ANDROID)
    EXPECT_TRUE(PathService::Get(base::DIR_SOURCE_ROOT, &module_dir));
#else
    EXPECT_TRUE(PathService::Get(base::DIR_MODULE, &module_dir));
#endif
    return module_dir.Append(path);
  }

 protected:
  std::unique_ptr<MediaEngagementPreloadedList> preloaded_list_;
};

TEST_F(MediaEngagementPreloadedListTest, CheckOriginIsPresent) {
  EXPECT_TRUE(LoadFromFile(GetFilePathRelativeToModule(kSampleDataPath)));

  EXPECT_TRUE(CheckOriginIsPresent(GURL("https://example.com")));
  EXPECT_TRUE(CheckOriginIsPresent(GURL("https://example.org:1234")));
  EXPECT_TRUE(CheckOriginIsPresent(GURL("https://test--3ya.com")));
  EXPECT_TRUE(CheckOriginIsPresent(GURL("http://123.123.123.123")));

  EXPECT_FALSE(CheckOriginIsPresent(GURL("https://example.org")));
  EXPECT_FALSE(CheckOriginIsPresent(GURL("http://example.com")));
  EXPECT_FALSE(CheckOriginIsPresent(GURL("http://123.123.123.124")));
}

TEST_F(MediaEngagementPreloadedListTest, LoadMissingFile) {
  EXPECT_FALSE(LoadFromFile(kMissingFilePath));
  EXPECT_FALSE(CheckOriginIsPresent(GURL("https://example.com")));
}

TEST_F(MediaEngagementPreloadedListTest, LoadBadFormatFile) {
  EXPECT_FALSE(LoadFromFile(kBadFormatFilePath));
  EXPECT_FALSE(CheckOriginIsPresent(GURL("https://example.com")));
}

TEST_F(MediaEngagementPreloadedListTest, LoadBadContentsFile) {
  EXPECT_FALSE(LoadFromFile(kBadContentsFilePath));
  EXPECT_FALSE(CheckOriginIsPresent(GURL("https://example.com")));
}
