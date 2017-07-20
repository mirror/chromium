// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/js_test_scripts.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {
namespace test {

// The format used to generate a JavaScript alert with a given message.
const char kAlertFormat[] = "(function() { alert(\"%s\"); })()";

base::string16 GetScriptForJavaScriptAlert(const std::string& message) {
  return base::UTF8ToUTF16(base::StringPrintf(kAlertFormat, message.c_str()));
}

}  // namespace test
}  // namespace web
