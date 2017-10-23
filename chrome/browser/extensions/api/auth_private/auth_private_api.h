// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_AUTH_PRIVATE_AUTH_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_AUTH_PRIVATE_AUTH_PRIVATE_API_H_

#include <string>

#include "base/compiler_specific.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "ui/message_center/notification_types.h"

namespace extensions {

class AuthPrivateSetUserForSigninFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("authPrivate.setUserForSignin",
                             AUTHPRIVATE_SETUSERFORSIGNIN)

 private:
  ~AuthPrivateSetUserForSigninFunction() override = default;
  ResponseAction Run() override;
};

// Don't kill the browser when we're in a browser test.
void SetAuthPrivateTest();

// The profile-keyed service that manages the authPrivate extension API.
class AuthPrivateAPI : public BrowserContextKeyedAPI {
 public:
  static BrowserContextKeyedAPIFactory<AuthPrivateAPI>* GetFactoryInstance();

  // TODO(achuith): Replace these with a mock object for system calls.
  bool test_mode() const { return test_mode_; }
  void set_test_mode(bool test_mode) { test_mode_ = test_mode; }

 private:
  friend class BrowserContextKeyedAPIFactory<AuthPrivateAPI>;

  AuthPrivateAPI() = default;
  ~AuthPrivateAPI() override = default;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "AuthPrivateAPI"; }
  static const bool kServiceIsNULLWhileTesting = true;
  static const bool kServiceRedirectedInIncognito = true;

  // true for ExtensionApiTest.AuthPrivate browser test.
  bool test_mode_ = false;
};

template <>
KeyedService*
BrowserContextKeyedAPIFactory<AuthPrivateAPI>::BuildServiceInstanceFor(
    content::BrowserContext* context) const;

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_AUTH_PRIVATE_AUTH_PRIVATE_API_H_
