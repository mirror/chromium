// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ABOUT_UI_CREDIT_UTILS_H_
#define COMPONENTS_ABOUT_UI_CREDIT_UTILS_H_

#include <jni.h>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/base_export.h"

namespace about_ui {

// Decode a Brotli compressed HTML license file and attach .js files.
BASE_EXPORT void GetCredits(std::string& response, bool isInline);

bool RegisterAboutUIUtils(JNIEnv* env);

}  // namespace about_ui

#endif  // COMPONENTS_ABOUT_UI_CREDIT_UTILS_H_
