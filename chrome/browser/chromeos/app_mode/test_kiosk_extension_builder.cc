// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/app_mode/test_kiosk_extension_builder.h"

#include "base/values.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"

using extensions::DictionaryBuilder;
using extensions::ListBuilder;
using extensions::ExtensionBuilder;

namespace chromeos {

TestKioskExtensionBuilder::TestKioskExtensionBuilder(
    const std::string& extension_id)
    : extension_id_(extension_id) {}

TestKioskExtensionBuilder::~TestKioskExtensionBuilder() = default;

void TestKioskExtensionBuilder::AddSecondaryExtension(const std::string& id) {
  secondary_extensions_.push_back(id);
}

scoped_refptr<const extensions::Extension> TestKioskExtensionBuilder::Build() {
  std::unique_ptr<base::DictionaryValue> background =
      DictionaryBuilder()
          .Set("scripts", ListBuilder().Append("background.js").Build())
          .Build();

  DictionaryBuilder manifest_builder;
  manifest_builder.Set("name", "Test kiosk app")
      .Set("version", version_.value_or("1.0"))
      .Set("manifest_version", 2)
      .Set(
          "app",
          DictionaryBuilder().Set("background", std::move(background)).Build());

  if (kiosk_enabled_)
    manifest_builder.SetBoolean("kiosk_enabled", kiosk_enabled_);

  manifest_builder.SetBoolean("offline_enabled", offline_enabled_);

  if (!secondary_extensions_.empty()) {
    ListBuilder secondary_extension_list_builder;
    for (const auto& secondary_extension : secondary_extensions_) {
      secondary_extension_list_builder.Append(
          DictionaryBuilder().Set("id", secondary_extension).Build());
    }
    manifest_builder.Set("kiosk_secondary_apps",
                         secondary_extension_list_builder.Build());
  }

  return ExtensionBuilder()
      .SetManifest(manifest_builder.Build())
      .SetID(extension_id_)
      .Build();
}

}  // namespace chromeos
