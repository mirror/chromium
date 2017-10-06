// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_handlers/app_theme_color_info.h"

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "extensions/common/image_util.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;

namespace {

const AppThemeColorInfo& GetInfo(const Extension* extension) {
  CR_DEFINE_STATIC_LOCAL(const AppThemeColorInfo, fallback, ());

  AppThemeColorInfo* info = static_cast<AppThemeColorInfo*>(
      extension->GetManifestData(keys::kAppThemeColor));
  return info ? *info : fallback;
}

}  // namespace

AppThemeColorInfo::AppThemeColorInfo() {}

AppThemeColorInfo::~AppThemeColorInfo() {}

// static
base::Optional<SkColor> AppThemeColorInfo::GetThemeColor(
    const Extension* extension) {
  return GetInfo(extension).theme_color;
}

AppThemeColorHandler::AppThemeColorHandler() {}

AppThemeColorHandler::~AppThemeColorHandler() {}

bool AppThemeColorHandler::Parse(Extension* extension, base::string16* error) {
  if (!extension->manifest()->HasPath(keys::kAppThemeColor))
    return true;

  std::string theme_color_string;
  SkColor theme_color = SK_ColorTRANSPARENT;
  if (!extension->manifest()->GetString(keys::kAppThemeColor,
                                        &theme_color_string) ||
      !base::StringToUint(theme_color_string, &theme_color)) {
    *error =
        base::UTF8ToUTF16(extensions::manifest_errors::kInvalidAppThemeColor);
    return false;
  }

  if (!extension->from_bookmark()) {
    *error = base::ASCIIToUTF16(
        extensions::manifest_errors::kInvalidThemeColorAppType);
    return false;
  }

  auto app_theme_color_info = std::make_unique<AppThemeColorInfo>();
  app_theme_color_info->theme_color = static_cast<SkColor>(theme_color);
  extension->SetManifestData(keys::kAppThemeColor,
                             std::move(app_theme_color_info));

  return true;
}

const std::vector<std::string> AppThemeColorHandler::Keys() const {
  return SingleKey(keys::kAppThemeColor);
}

}  // namespace extensions
