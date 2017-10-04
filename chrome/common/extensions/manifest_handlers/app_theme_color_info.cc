// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_handlers/app_theme_color_info.h"

#include <memory>

#include "base/lazy_instance.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "extensions/common/image_util.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;

namespace {

static base::LazyInstance<AppThemeColorInfo>::DestructorAtExit
    g_empty_app_theme_color_info = LAZY_INSTANCE_INITIALIZER;

const AppThemeColorInfo& GetInfo(const Extension* extension) {
  AppThemeColorInfo* info = static_cast<AppThemeColorInfo*>(
      extension->GetManifestData(keys::kAppThemeColor));
  return info ? *info : g_empty_app_theme_color_info.Get();
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
  auto app_theme_color_info = std::make_unique<AppThemeColorInfo>();

  int theme_color;
  if (extension->manifest()->GetInteger(keys::kAppThemeColor, &theme_color))
    app_theme_color_info->theme_color = theme_color;

  extension->SetManifestData(keys::kAppThemeColor,
                             std::move(app_theme_color_info));
  return true;
}

const std::vector<std::string> AppThemeColorHandler::Keys() const {
  return SingleKey(keys::kAppThemeColor);
}

}  // namespace extensions
