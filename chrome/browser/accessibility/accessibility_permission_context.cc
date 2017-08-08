// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/accessibility/accessibility_permission_context.h"

AccessibilityPermissionContext::AccessibilityPermissionContext(Profile* profile)
    : PermissionContextBase(
          profile,
          CONTENT_SETTINGS_TYPE_ACCESSIBILITY_EVENTS,
          blink::WebFeaturePolicyFeature::kAccessibilityEvents) {}

AccessibilityPermissionContext::~AccessibilityPermissionContext() {}

ContentSetting AccessibilityPermissionContext::GetPermissionStatusInternal(
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  return PermissionContextBase::GetPermissionStatusInternal(
      render_frame_host, requesting_origin, embedding_origin);
}

bool AccessibilityPermissionContext::IsRestrictedToSecureOrigins() const {
  return false;
}
