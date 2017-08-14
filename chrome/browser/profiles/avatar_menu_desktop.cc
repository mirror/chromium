// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/avatar_menu.h"

#include <stddef.h>

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "ui/base/resource/resource_bundle.h"

// static
AvatarMenu::ImageLoadStatus AvatarMenu::GetImageForMenuButton(
    const base::FilePath& profile_path,
    gfx::Image* image) {
  ImageLoadStatus status = ImageLoadStatus::SUCCESS_LOAD_DEFAULT;

  ProfileAttributesEntry* entry;
  if (!g_browser_process->profile_manager()->GetProfileAttributesStorage().
          GetProfileAttributesWithPath(profile_path, &entry)) {
    // This can happen if the user deletes the current profile.
    return ImageLoadStatus::FAIL_AS_PROFILE_DELETED;
  }

  // If there is a Gaia image available, try to use that.
  if (entry->IsUsingGAIAPicture()) {
    // TODO(chengx): The GetGAIAPicture API call will trigger an async image
    // load from disk if it has not been loaded. This is non-obvious and
    // dependency should be avoided. We should come with a better idea to handle
    // this.
    const gfx::Image* gaia_image = entry->GetGAIAPicture();

    if (gaia_image) {
      *image = *gaia_image;
      return ImageLoadStatus::SUCCESS_LOAD_GAIA;
    }
    if (entry->IsGAIAPictureLoaded())
      status = ImageLoadStatus::USE_DEFAULT_AS_GAIA_MISSING;
    else
      status = ImageLoadStatus::USE_DEFAULT_AS_GAIA_LOADING;
  }

  // Otherwise, use the default resource, not the downloaded high-res one.
  const size_t icon_index = entry->GetAvatarIconIndex();
  const int resource_id =
      profiles::GetDefaultAvatarIconResourceIDAtIndex(icon_index);
  *image = ResourceBundle::GetSharedInstance().GetNativeImageNamed(resource_id);

  return status;
}
