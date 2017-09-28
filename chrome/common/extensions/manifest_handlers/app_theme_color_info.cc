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

AppThemeColorInfo::AppThemeColorInfo() : theme_color_(SK_ColorTRANSPARENT) {}

AppThemeColorInfo::~AppThemeColorInfo() {}

// static
SkColor AppThemeColorInfo::GetThemeColor(const Extension* extension) {
  return GetInfo(extension).theme_color_;
}

// static
const std::string& AppThemeColorInfo::GetThemeColorString(
    const Extension* extension) {
  return GetInfo(extension).theme_color_string_;
}

AppThemeColorHandler::AppThemeColorHandler() {}

AppThemeColorHandler::~AppThemeColorHandler() {}

bool AppThemeColorHandler::Parse(Extension* extension, base::string16* error) {
  std::unique_ptr<AppThemeColorInfo> app_theme_color_info(
      new AppThemeColorInfo);

  const base::Value* temp = nullptr;
  if (extension->manifest()->Get(keys::kAppThemeColor, &temp)) {
    if (!temp->GetAsString(&app_theme_color_info->theme_color_string_)) {
      *error =
          base::UTF8ToUTF16(extensions::manifest_errors::kInvalidAppThemeColor);
      return false;
    }

    if (!image_util::ParseHexColorString(
            app_theme_color_info->theme_color_string_,
            &app_theme_color_info->theme_color_)) {
      *error =
          base::UTF8ToUTF16(extensions::manifest_errors::kInvalidAppThemeColor);
      return false;
    }
  }

  extension->SetManifestData(keys::kAppThemeColor,
                             std::move(app_theme_color_info));
  return true;
}

const std::vector<std::string> AppThemeColorHandler::Keys() const {
  return SingleKey(keys::kAppThemeColor);
}

}  // namespace extensions
