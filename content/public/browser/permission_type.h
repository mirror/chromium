// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_PERMISSION_TYPE_H_
#define CONTENT_PUBLIC_BROWSER_PERMISSION_TYPE_H_

namespace content {

// This enum is also used for UMA purposes, so it needs to adhere to
// the UMA guidelines.
// Make sure you update histograms.xml if you add new permission types.
// Never delete or reorder an entry; only add new entries
// immediately before PermissionType::NUM
enum class PermissionType {
  MIDI_SYSEX = 1,
  NOTIFICATIONS = 2,
  GEOLOCATION = 3,
  PROTECTED_MEDIA_IDENTIFIER = 4,
  MIDI = 5,
  DURABLE_STORAGE = 6,
  AUDIO_CAPTURE = 7,
  VIDEO_CAPTURE = 8,
  BACKGROUND_SYNC = 9,
  FLASH = 10,
  SENSORS = 11,
  ACCESSIBILITY_EVENTS = 12,

  // Always keep this at the end.
  NUM,
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_PERMISSION_TYPE_H_
