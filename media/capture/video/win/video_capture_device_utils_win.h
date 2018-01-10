// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_UTILS_WIN_H_
#define MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_UTILS_WIN_H_

#include <windows.h>

namespace media {

// Returns the rotation of the camera.
// Note: Because only UWP has APIs to detect camera location (front/back or
// external USB camera), which is not available in Chromium yet, assume it's
// always front camera when auto rotation is enabled for now.
int GetCameraRotation();

// Returns true if auto rotation is enabled on this device.
// A Windows device (i.e. 2-in-1 laptops) may switch between laptop and tablet
// mode, or user may toggles auto-rotation on/off. So only check device
// orientation when auto-rotation is enabled on the target device.
bool IsAutoRotationEnabled();

// Returns true if target device has active internal display panel, e.g. the
// screen attached to tablets or laptops, and stores its device info in
// |internal_display_device|.
// When display is only on external monitor, the auto-rotation state still may
// be ENALBED on the target device. In that case, we shouldn't query the display
// orientation and the built-in camera will be treated as an external one.
bool HasActiveInternalDisplayDevice(DISPLAY_DEVICE* internal_display_device);

// Returns S_OK if the path info of the target display device shows it is an
// internal display panel.
HRESULT CheckPathInfoForInternal(const PCWSTR device_name);

// Returns true if this is an integrated display panel.
bool IsInternalVideoOutput(
    const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY video_output_tech_type);
}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_UTILS_WIN_H_