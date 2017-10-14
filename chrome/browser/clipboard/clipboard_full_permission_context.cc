// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/clipboard/clipboard_full_permission_context.h"
ClipboardFullPermissionContext::ClipboardFullPermissionContext(Profile* profile)
    : PermissionContextBase(profile,
                            CONTENT_SETTINGS_TYPE_CLIPBOARD_FULL,
                            blink::WebFeaturePolicyFeature::kClipboardFull) {}
ClipboardFullPermissionContext::~ClipboardFullPermissionContext() = default;
bool ClipboardFullPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}
