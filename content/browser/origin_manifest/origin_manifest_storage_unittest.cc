// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_storage.h"
#include "content/browser/origin_manifest/origin_manifest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class OriginManifestStorageTest : public testing::Test {};

TEST_F(OriginManifestStorageTest, AccessOriginManifest) {
  struct TestCase {
    const std::string origin;
    const std::string version;
    std::unique_ptr<content::OriginManifest> manifest;
  } test{"origin1", "version1", OriginManifest::CreateOriginManifest("{}")};

  OriginManifest* test_manifest = test.manifest.get();

  EXPECT_FALSE(OriginManifestStorage::HasManifest(test.origin, test.version));
  EXPECT_EQ(OriginManifestStorage::GetVersion(test.origin), "1");
  EXPECT_EQ(OriginManifestStorage::GetManifest(test.origin, test.version),
            nullptr);

  OriginManifestStorage::Add(test.origin, test.version,
                             std::move(test.manifest));

  EXPECT_TRUE(OriginManifestStorage::HasManifest(test.origin, test.version));
  EXPECT_EQ(OriginManifestStorage::GetVersion(test.origin), test.version);
  EXPECT_EQ(OriginManifestStorage::GetManifest(test.origin, test.version),
            test_manifest);

  OriginManifestStorage::Revoke(test.origin);

  EXPECT_FALSE(OriginManifestStorage::HasManifest(test.origin, test.version));
  EXPECT_EQ(OriginManifestStorage::GetVersion(test.origin), "1");
  EXPECT_EQ(OriginManifestStorage::GetManifest(test.origin, test.version),
            nullptr);
}

TEST_F(OriginManifestStorageTest, InvalidateOriginManifest) {
  struct TestCase {
    const std::string origin;
    const std::string version;
    std::unique_ptr<OriginManifest> manifest;
  };
  TestCase man1 = {"origin1", "version1",
                   OriginManifest::CreateOriginManifest("{}")};
  TestCase man2 = {"origin1", "version2",
                   OriginManifest::CreateOriginManifest("{}")};
  TestCase man3 = {"origin2", "version2",
                   OriginManifest::CreateOriginManifest("{}")};

  OriginManifestStorage::Add(man1.origin, man1.version,
                             std::move(man1.manifest));

  EXPECT_TRUE(OriginManifestStorage::HasManifest(man1.origin, man1.version));

  // replace policy for same origin with different version
  OriginManifestStorage::Add(man2.origin, man2.version,
                             std::move(man2.manifest));

  EXPECT_FALSE(OriginManifestStorage::HasManifest(man1.origin, man1.version));
  EXPECT_TRUE(OriginManifestStorage::HasManifest(man2.origin, man2.version));

  // add policy with same version identifier for different origin
  OriginManifestStorage::Add(man3.origin, man3.version,
                             std::move(man3.manifest));

  EXPECT_TRUE(OriginManifestStorage::HasManifest(man2.origin, man2.version));
  EXPECT_TRUE(OriginManifestStorage::HasManifest(man3.origin, man3.version));

  OriginManifestStorage::Revoke(man2.origin);
  OriginManifestStorage::Revoke(man3.origin);
}

}  // namespace content
