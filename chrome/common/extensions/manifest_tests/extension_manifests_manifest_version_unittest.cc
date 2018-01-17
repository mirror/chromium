// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "chrome/common/extensions/manifest_tests/chrome_manifest_test.h"
#include "extensions/common/manifest_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using extensions::Extension;

namespace errors = extensions::manifest_errors;

TEST_F(ChromeManifestTest, ManifestVersionError) {
  std::unique_ptr<base::DictionaryValue> manifest1(new base::DictionaryValue());
  manifest1->SetKey("name", base::Value("Miles"));
  manifest1->SetKey("version", base::Value("0.55"));

  std::unique_ptr<base::DictionaryValue> manifest2(manifest1->DeepCopy());
  manifest2->SetKey("manifest_version", base::Value(1));

  std::unique_ptr<base::DictionaryValue> manifest3(manifest1->DeepCopy());
  manifest3->SetKey("manifest_version", base::Value(2));

  struct {
    const char* test_name;
    bool require_modern_manifest_version;
    base::DictionaryValue* manifest;
    bool expect_error;
  } test_data[] = {
      {"require_modern_with_default", true, manifest1.get(), true},
      {"require_modern_with_v1", true, manifest2.get(), true},
      {"require_modern_with_v2", true, manifest3.get(), false},
      {"dont_require_modern_with_default", false, manifest1.get(), false},
      {"dont_require_modern_with_v1", false, manifest2.get(), false},
      {"dont_require_modern_with_v2", false, manifest3.get(), false},
  };

  for (auto& entry : test_data) {
    int create_flags = Extension::NO_FLAGS;
    if (entry.require_modern_manifest_version)
      create_flags |= Extension::REQUIRE_MODERN_MANIFEST_VERSION;
    if (entry.expect_error) {
      LoadAndExpectError(ManifestData(entry.manifest, entry.test_name),
                         errors::kInvalidManifestVersionOld,
                         extensions::Manifest::UNPACKED, create_flags);
    } else {
      LoadAndExpectSuccess(ManifestData(entry.manifest, entry.test_name),
                           extensions::Manifest::UNPACKED, create_flags);
    }
  }
}
