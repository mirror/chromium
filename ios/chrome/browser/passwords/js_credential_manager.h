// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_

#include "ios/chrome/browser/procedural_block_types.h"
#import "ios/web/public/web_state/js/crw_js_injection_manager.h"

// Class responsible for communicating from browser to the website.
//
// TODO(crbug.com/435048) tgarbus: JSCredentialManager should
// implement methods to be called from CredentialManager. Each of those
// methods should be responsible for resolving or rejecting a promise
// with the result passed from CredentialManager.
@interface JSCredentialManager : CRWJSInjectionManager

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
