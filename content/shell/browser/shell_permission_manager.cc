// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_permission_manager.h"

#include "base/callback.h"
#include "base/command_line.h"
#include "content/public/browser/permission_type.h"
#include "content/public/common/content_switches.h"
#include "media/base/media_switches.h"

namespace content {

ShellPermissionManager::ShellPermissionManager()
    : PermissionManager() {
}

ShellPermissionManager::~ShellPermissionManager() {
}

int ShellPermissionManager::RequestPermission(
    PermissionType permission,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::Callback<void(mojom::PermissionStatus)>& callback) {
  callback.Run(permission == PermissionType::GEOLOCATION
                   ? mojom::PermissionStatus::GRANTED
                   : mojom::PermissionStatus::DENIED);
  return kNoPendingOperation;
}

int ShellPermissionManager::RequestPermissions(
    const std::vector<PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::Callback<void(const std::vector<mojom::PermissionStatus>&)>&
        callback) {
  std::vector<mojom::PermissionStatus> result(permissions.size());
  for (const auto& permission : permissions) {
    result.push_back(permission == PermissionType::GEOLOCATION
                         ? mojom::PermissionStatus::GRANTED
                         : mojom::PermissionStatus::DENIED);
  }
  callback.Run(result);
  return kNoPendingOperation;
}

void ShellPermissionManager::CancelPermissionRequest(int request_id) {
}

void ShellPermissionManager::ResetPermission(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

mojom::PermissionStatus ShellPermissionManager::GetPermissionStatus(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  // Background sync browser tests require permission to be granted by default.
  // TODO(nsatragno): add a command line flag so that it's only granted for
  // tests.
  if (permission == PermissionType::BACKGROUND_SYNC)
    return mojom::PermissionStatus::GRANTED;

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if ((permission == PermissionType::AUDIO_CAPTURE ||
       permission == PermissionType::VIDEO_CAPTURE) &&
      command_line->HasSwitch(switches::kUseFakeDeviceForMediaStream) &&
      command_line->HasSwitch(switches::kUseFakeUIForMediaStream)) {
    return mojom::PermissionStatus::GRANTED;
  }
  return mojom::PermissionStatus::DENIED;
}

void ShellPermissionManager::RegisterPermissionUsage(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

int ShellPermissionManager::SubscribePermissionStatusChange(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const base::Callback<void(mojom::PermissionStatus)>& callback) {
  return kNoPendingOperation;
}

void ShellPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {
}

}  // namespace content
