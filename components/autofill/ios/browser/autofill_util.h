// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_UTIL_H_
#define COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_UTIL_H_

namespace web {
class WebState;
}  // namespace web

namespace autofill {

// Checks if current context is secure from an autofill standpoint.
bool IsContextSecureForWebState(web::WebState* web_state);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_IOS_BROWSER_AUTOFILL_UTIL_H_
