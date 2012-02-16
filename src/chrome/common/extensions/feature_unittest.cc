// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/feature.h"

#include "testing/gtest/include/gtest/gtest.h"

using extensions::Feature;

namespace {

struct IsAvailableTestData {
  std::string extension_id;
  Extension::Type extension_type;
  Feature::Context context;
  Feature::Location location;
  Feature::Platform platform;
  int manifest_version;
  bool expected_result;
};

}  // namespace

TEST(ExtensionFeatureTest, IsAvailableNullCase) {
  const IsAvailableTestData tests[] = {
    { "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_CONTEXT,
      Feature::UNSPECIFIED_LOCATION, Feature::UNSPECIFIED_PLATFORM, -1, true },
    { "random-extension", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_CONTEXT,
      Feature::UNSPECIFIED_LOCATION, Feature::UNSPECIFIED_PLATFORM, -1, true },
    { "", Extension::TYPE_PACKAGED_APP, Feature::UNSPECIFIED_CONTEXT,
      Feature::UNSPECIFIED_LOCATION, Feature::UNSPECIFIED_PLATFORM, -1, true },
    { "", Extension::TYPE_UNKNOWN, Feature::PRIVILEGED_CONTEXT,
      Feature::UNSPECIFIED_LOCATION, Feature::UNSPECIFIED_PLATFORM, -1, true },
    { "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_CONTEXT,
      Feature::COMPONENT_LOCATION, Feature::UNSPECIFIED_PLATFORM, -1, true },
    { "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_CONTEXT,
      Feature::UNSPECIFIED_LOCATION, Feature::CHROMEOS_PLATFORM, -1, true },
    { "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_CONTEXT,
      Feature::UNSPECIFIED_LOCATION, Feature::UNSPECIFIED_PLATFORM, 25, true }
  };

  Feature feature;
  for (size_t i = 0; i < arraysize(tests); ++i) {
    const IsAvailableTestData& test = tests[i];
    EXPECT_EQ(test.expected_result,
              feature.IsAvailable(test.extension_id, test.extension_type,
                                  test.location, test.context, test.platform,
                                  test.manifest_version));
  }
}

TEST(ExtensionFeatureTest, Whitelist) {
  Feature feature;
  feature.whitelist()->insert("foo");
  feature.whitelist()->insert("bar");

  EXPECT_TRUE(feature.IsAvailable(
      "foo", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
  EXPECT_TRUE(feature.IsAvailable(
      "bar", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));

  EXPECT_FALSE(feature.IsAvailable(
      "baz", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));

  feature.extension_types()->insert(Extension::TYPE_PACKAGED_APP);
  EXPECT_FALSE(feature.IsAvailable(
      "baz", Extension::TYPE_PACKAGED_APP, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
}

TEST(ExtensionFeatureTest, PackageType) {
  Feature feature;
  feature.extension_types()->insert(Extension::TYPE_EXTENSION);
  feature.extension_types()->insert(Extension::TYPE_PACKAGED_APP);

  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_EXTENSION, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_PACKAGED_APP, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));

  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_THEME, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
}

TEST(ExtensionFeatureTest, Context) {
  Feature feature;
  feature.contexts()->insert(Feature::PRIVILEGED_CONTEXT);
  feature.contexts()->insert(Feature::CONTENT_SCRIPT_CONTEXT);

  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::PRIVILEGED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::CONTENT_SCRIPT_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));

  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNPRIVILEGED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
}

TEST(ExtensionFeatureTest, Location) {
  Feature feature;
  feature.set_location(Feature::COMPONENT_LOCATION);
  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::COMPONENT_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
}

TEST(ExtensionFeatureTest, Platform) {
  Feature feature;
  feature.set_platform(Feature::CHROMEOS_PLATFORM);
  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::CHROMEOS_PLATFORM, -1));
  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, -1));
}

TEST(ExtensionFeatureTest, Version) {
  Feature feature;
  feature.set_min_manifest_version(5);

  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, 0));
  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, 4));

  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, 5));
  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, 10));

  feature.set_max_manifest_version(8);

  EXPECT_FALSE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, 10));
  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, 8));
  EXPECT_TRUE(feature.IsAvailable(
      "", Extension::TYPE_UNKNOWN, Feature::UNSPECIFIED_LOCATION,
      Feature::UNSPECIFIED_CONTEXT, Feature::UNSPECIFIED_PLATFORM, 7));
}

TEST(ExtensionFeatureTest, ParseNull) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  scoped_ptr<Feature> feature(Feature::Parse(value.get()));
  EXPECT_TRUE(feature->whitelist()->empty());
  EXPECT_TRUE(feature->extension_types()->empty());
  EXPECT_TRUE(feature->contexts()->empty());
  EXPECT_EQ(Feature::UNSPECIFIED_LOCATION, feature->location());
  EXPECT_EQ(Feature::UNSPECIFIED_PLATFORM, feature->platform());
  EXPECT_EQ(0, feature->min_manifest_version());
  EXPECT_EQ(0, feature->max_manifest_version());
}

TEST(ExtensionFeatureTest, ParseWhitelist) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  ListValue* whitelist = new ListValue();
  whitelist->Append(Value::CreateStringValue("foo"));
  whitelist->Append(Value::CreateStringValue("bar"));
  value->Set("whitelist", whitelist);
  scoped_ptr<Feature> feature(Feature::Parse(value.get()));
  EXPECT_EQ(2u, feature->whitelist()->size());
  EXPECT_TRUE(feature->whitelist()->count("foo"));
  EXPECT_TRUE(feature->whitelist()->count("bar"));
}

TEST(ExtensionFeatureTest, ParsePackageTypes) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  ListValue* extension_types = new ListValue();
  extension_types->Append(Value::CreateStringValue("extension"));
  extension_types->Append(Value::CreateStringValue("theme"));
  extension_types->Append(Value::CreateStringValue("packaged_app"));
  extension_types->Append(Value::CreateStringValue("hosted_app"));
  extension_types->Append(Value::CreateStringValue("platform_app"));
  value->Set("package_type", extension_types);
  scoped_ptr<Feature> feature(Feature::Parse(value.get()));
  EXPECT_EQ(5u, feature->extension_types()->size());
  EXPECT_TRUE(feature->extension_types()->count(Extension::TYPE_EXTENSION));
  EXPECT_TRUE(feature->extension_types()->count(Extension::TYPE_THEME));
  EXPECT_TRUE(feature->extension_types()->count(Extension::TYPE_PACKAGED_APP));
  EXPECT_TRUE(feature->extension_types()->count(Extension::TYPE_HOSTED_APP));
  EXPECT_TRUE(feature->extension_types()->count(Extension::TYPE_PLATFORM_APP));

  extension_types->Clear();
  extension_types->Append(Value::CreateStringValue("all"));
  scoped_ptr<Feature> feature2(Feature::Parse(value.get()));
  EXPECT_EQ(*(feature->extension_types()), *(feature2->extension_types()));
}

TEST(ExtensionFeatureTest, ParseContexts) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  ListValue* contexts = new ListValue();
  contexts->Append(Value::CreateStringValue("privileged"));
  contexts->Append(Value::CreateStringValue("unprivileged"));
  contexts->Append(Value::CreateStringValue("content_script"));
  value->Set("contexts", contexts);
  scoped_ptr<Feature> feature(Feature::Parse(value.get()));
  EXPECT_EQ(3u, feature->contexts()->size());
  EXPECT_TRUE(feature->contexts()->count(Feature::PRIVILEGED_CONTEXT));
  EXPECT_TRUE(feature->contexts()->count(Feature::UNPRIVILEGED_CONTEXT));
  EXPECT_TRUE(feature->contexts()->count(Feature::CONTENT_SCRIPT_CONTEXT));

  contexts->Clear();
  contexts->Append(Value::CreateStringValue("all"));
  scoped_ptr<Feature> feature2(Feature::Parse(value.get()));
  EXPECT_EQ(*(feature->contexts()), *(feature2->contexts()));
}

TEST(ExtensionFeatureTest, ParseLocation) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  value->SetString("location", "component");
  scoped_ptr<Feature> feature(Feature::Parse(value.get()));
  EXPECT_EQ(Feature::COMPONENT_LOCATION, feature->location());
}

TEST(ExtensionFeatureTest, ParsePlatform) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  value->SetString("platform", "chromeos");
  scoped_ptr<Feature> feature(Feature::Parse(value.get()));
  EXPECT_EQ(Feature::CHROMEOS_PLATFORM, feature->platform());
}

TEST(ExtensionFeatureTest, ManifestVersion) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  value->SetInteger("min_manifest_version", 1);
  value->SetInteger("max_manifest_version", 5);
  scoped_ptr<Feature> feature(Feature::Parse(value.get()));
  EXPECT_EQ(1, feature->min_manifest_version());
  EXPECT_EQ(5, feature->max_manifest_version());
}
