// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/resource/resource_bundle.h"

#include <string>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/stringprintf.h"
#include "ui/base/resource/resource_handle.h"

namespace {

FilePath GetResourcesPakFilePath(const std::string& pak_name) {
  FilePath path;
  if (PathService::Get(base::DIR_ANDROID_APP_DATA, &path))
    return path.AppendASCII("paks").AppendASCII(pak_name.c_str());

  // Return just the name of the pack file.
  return FilePath(pak_name.c_str());
}

}  // namespace

namespace ui {

void ResourceBundle::LoadCommonResources() {
  AddDataPack(GetResourcesPakFilePath("chrome.pak"),
              SCALE_FACTOR_100P);
  AddDataPack(GetResourcesPakFilePath("theme_resources_standard.pak"),
              SCALE_FACTOR_100P);
  AddDataPack(GetResourcesPakFilePath("ui_resources_standard.pak"),
              SCALE_FACTOR_100P);
}

gfx::Image& ResourceBundle::GetNativeImageNamed(int resource_id, ImageRTL rtl) {
  // Flipped image is not used on Android.
  DCHECK_EQ(rtl, RTL_DISABLED);
  return GetImageNamed(resource_id);
}

}
