// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_GVR_KEYBOARD_LOADER_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_GVR_KEYBOARD_LOADER_H_

// Dynamically loads the Keyboard API from the Daydream APK.
// @return True if loading was successful.
bool load_gvr_sdk();

// Closes the API opened using the load_gvr_sdk() function above.
void close_gvr_sdk();

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_GVR_KEYBOARD_LOADER_H_
