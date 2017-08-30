// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest.h"
#include "content/browser/origin_manifest/origin_manifest_store.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class OriginManifestStoreTest : public testing::Test {};

TEST_F(OriginManifestStoreTest, AccessOriginManifest) {
  OriginManifestStore& store = OriginManifestStore::Get();
  std::string origin = "https://a.com";
  std::string version = "v1";
  std::unique_ptr<content::OriginManifest> test_manifest =
      OriginManifest::CreateOriginManifest(origin, version, "{}");
  content::OriginManifest* test_manifest_ptr = test_manifest.get();

  EXPECT_EQ(store.GetVersion(origin), "1");
  EXPECT_FALSE(store.HasManifest(origin, version));
  EXPECT_EQ(store.GetManifest(origin, version), nullptr);

  store.Add(origin, version, std::move(test_manifest));

  EXPECT_TRUE(store.HasManifest(origin, version));
  EXPECT_EQ(store.GetVersion(origin), version);
  EXPECT_EQ(store.GetManifest(origin, version), test_manifest_ptr);

  store.Revoke(origin);
  // Note: do NOT use test_manifest_ptr anymore. Revoke destroys it.

  EXPECT_FALSE(store.HasManifest(origin, version));
  EXPECT_EQ(store.GetVersion(origin), "1");
  EXPECT_EQ(store.GetManifest(origin, version), nullptr);
}

TEST_F(OriginManifestStoreTest, InvalidateOriginManifest) {
  OriginManifestStore& store = OriginManifestStore::Get();
  struct {
    std::string origin;
    std::string version;
    std::string json;
  } testData[]{
      {"origin1", "version1", "{}"},
      {"origin1", "version2", "{}"},
      {"origin2", "version2", "{}"},
  };
  std::unique_ptr<content::OriginManifest> test_manifest[]{
      OriginManifest::CreateOriginManifest(
          testData[0].origin, testData[0].version, testData[0].json),
      OriginManifest::CreateOriginManifest(
          testData[1].origin, testData[1].version, testData[1].json),
      OriginManifest::CreateOriginManifest(
          testData[2].origin, testData[2].version, testData[2].json)};

  store.Add(testData[0].origin, testData[0].version,
            std::move(test_manifest[0]));

  EXPECT_TRUE(store.HasManifest(testData[0].origin, testData[0].version));

  // replace policy for same origin with different version
  store.Add(testData[1].origin, testData[1].version,
            std::move(test_manifest[1]));

  EXPECT_FALSE(store.HasManifest(testData[0].origin, testData[0].version));
  EXPECT_TRUE(store.HasManifest(testData[1].origin, testData[1].version));

  // add policy with same version identifier for different origin
  store.Add(testData[2].origin, testData[2].version,
            std::move(test_manifest[2]));

  EXPECT_TRUE(store.HasManifest(testData[1].origin, testData[1].version));
  EXPECT_TRUE(store.HasManifest(testData[2].origin, testData[2].version));

  store.Revoke(testData[1].origin);
  store.Revoke(testData[2].origin);
}

}  // namespace content
