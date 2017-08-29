// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/test_content_client.h"

#include <utility>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_piece.h"
#include "build/build_config.h"

#if defined(OS_ANDROID)
#include "base/android/apk_assets.h"
#endif

#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_handle.h"

namespace content {

TestContentClient::TestContentClient() {
  // content_shell.pak is not built on iOS as it is not required.
  base::FilePath content_shell_pack_path;
  base::File pak_file;
  base::MemoryMappedFile::Region pak_region;

#if defined(OS_ANDROID)
  // Tests that don't yet use .isolate files require loading from within .apk.
  pak_file = base::File(
      base::android::OpenApkAsset("assets/content_shell.pak", &pak_region));

  // on Android all pak files are inside the paks folder.
  PathService::Get(base::DIR_ANDROID_APP_DATA, &content_shell_pack_path);
  content_shell_pack_path = content_shell_pack_path.Append(
      FILE_PATH_LITERAL("paks"));
#else
  PathService::Get(base::DIR_MODULE, &content_shell_pack_path);
#endif  // defined(OS_ANDROID)

  if (pak_file.IsValid()) {
    ui::ResourceBundle::InitSharedInstanceWithPakFileRegion(std::move(pak_file),
                                                            pak_region);
  } else {
    content_shell_pack_path = content_shell_pack_path.Append(
        FILE_PATH_LITERAL("content_shell.pak"));
    ui::ResourceBundle::InitSharedInstanceWithPakPath(content_shell_pack_path);
  }
}

TestContentClient::~TestContentClient() {
}

std::string TestContentClient::GetUserAgent() const {
  return std::string("TestContentClient");
}

base::StringPiece TestContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

}  // namespace content
