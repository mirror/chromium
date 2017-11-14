// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/clipboard/clipboard_write_permission_context.h"

ClipboardWritePermissionContext::ClipboardWritePermissionContext(
    Profile* profile)
    : PermissionContextBase(profile,
                            CONTENT_SETTINGS_TYPE_CLIPBOARD_WRITE,
                            blink::FeaturePolicyFeature::kNotFound) {}
ClipboardWritePermissionContext::~ClipboardWritePermissionContext() = default;

bool ClipboardWritePermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}
