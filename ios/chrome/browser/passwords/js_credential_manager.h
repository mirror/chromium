// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_

#include "ios/chrome/browser/procedural_block_types.h"
#import "ios/web/public/web_state/js/crw_js_injection_manager.h"

// Injects the JavaScript that implements the Credential Manager API and
// provides and app-side interface for interacting with it.
// TODO(crbug.com/435048) tgarbus: JSCredentialManager should also listen for
// messages
//   from Javascript and invoke CredentialManager methods accordingly.
@interface JSCredentialManager : CRWJSInjectionManager

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
