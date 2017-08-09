// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_IMPORTANCE_H_
#define CONTENT_BROWSER_WEB_CONTENTS_IMPORTANCE_H_

namespace content {

// Importance of a WebContents that's independent from visibility.
// Values are listed in increasing importance.
//
// Note this is only used by and implemented on Android which exposes this API
// through public java code. If this useful on other platforms, then this enum
// and related methods on WebContentsImpl should be moved to the public
// interface.
//
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.content_public.browser
enum class WebContentsImportance {
  // NORMAL is the default value.
  NORMAL = 0,
  MODERATE,
  IMPORTANT,
  COUNT,
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_IMPORTANCE_H_
