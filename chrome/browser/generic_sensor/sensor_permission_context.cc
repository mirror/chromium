// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/generic_sensor/sensor_permission_context.h"

SensorPermissionContext::SensorPermissionContext(
    Profile* profile,
    ContentSettingsType content_settings_type)
    : PermissionContextBase(profile,
                            content_settings_type,
                            blink::WebFeaturePolicyFeature::kNotFound) {
  DCHECK(content_settings_type == CONTENT_SETTINGS_TYPE_ACCELEROMETER ||
         content_settings_type == CONTENT_SETTINGS_TYPE_AMBIENT_LIGHT_SENSOR ||
         content_settings_type == CONTENT_SETTINGS_TYPE_GYROSCOPE ||
         content_settings_type == CONTENT_SETTINGS_TYPE_MAGNETOMETER);
}

SensorPermissionContext::~SensorPermissionContext() {}

ContentSetting SensorPermissionContext::GetPermissionStatusInternal(
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  if (requesting_origin != embedding_origin)
    return CONTENT_SETTING_BLOCK;
  if (content_settings_type() == CONTENT_SETTINGS_TYPE_ACCELEROMETER ||
      content_settings_type() == CONTENT_SETTINGS_TYPE_GYROSCOPE) {
    return CONTENT_SETTING_ALLOW;
  }
  return CONTENT_SETTING_BLOCK;
}

bool SensorPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}
