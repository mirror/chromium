// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/clipboard/clipboard_permission_context.h"

ClipboardPermissionContext::ClipboardPermissionContext(Profile* profile)
    : PermissionContextBase(profile,
                            CONTENT_SETTINGS_TYPE_CLIPBOARD,
                            blink::WebFeaturePolicyFeature::kNotFound) {}
ClipboardPermissionContext::~ClipboardPermissionContext() = default;

bool ClipboardPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}
