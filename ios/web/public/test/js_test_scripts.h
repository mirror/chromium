// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_JS_TEST_SCRIPTS_H_
#define IOS_WEB_PUBLIC_TEST_JS_TEST_SCRIPTS_H_

#include "base/strings/string16.h"

namespace web {
namespace test {

// Returns a script string that will show a JavaScript alert with |message|.
base::string16 GetScriptForJavaScriptAlert(const std::string& message);

}  // namespace test
}  // namespace web
#endif  // IOS_WEB_PUBLIC_TEST_JS_TEST_SCRIPTS_H_
