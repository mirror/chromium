// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_CHROME_NATIVE_NOTIFICATION_PREP_WIN_H_
#define CHROME_APP_CHROME_NATIVE_NOTIFICATION_PREP_WIN_H_

// This switch allows the native notification functionality (activation part) to
// execute behind a flag.
// TODO(chengx): Remove when shipping this feature.
const char kEnableNativeNotification[] = "enable_native_notification";

void PrepareForNativeNotification();

#endif  // CHROME_APP_CHROME_NATIVE_NOTIFICATION_PREP_WIN_H_
