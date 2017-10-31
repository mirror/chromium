// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_CHROME_NATIVE_NOTIFICATION_PREP_WIN_H_
#define CHROME_APP_CHROME_NATIVE_NOTIFICATION_PREP_WIN_H_

// This switch allows the Windows 10 native notification features (activation
// part) to function behind a flag.
// TODO(chengx): Remove when shipping this feature.
extern const char kEnableNativeNotification[];

// Starts a thread to do preparing work for Windows 10 native notification
// features.
void StartNativeNotificationThread();

#endif  // CHROME_APP_CHROME_NATIVE_NOTIFICATION_PREP_WIN_H_
