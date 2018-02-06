// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_AUTH_PRIVATE_AUTH_PRIVATE_API_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_AUTH_PRIVATE_AUTH_PRIVATE_API_H_

#include <string>

#include "base/compiler_specific.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"

class AuthPrivateGetLoginStatusFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("authPrivate.getLoginStatus",
                             AUTHPRIVATE_GETLOGINSTATUS)

 private:
  ~AuthPrivateGetLoginStatusFunction() override = default;

  // UIThreadExtensionFunction:
  ResponseAction Run() override;
};

// The profile-keyed service that manages the authPrivate extension API.
class AuthPrivateAPI : public extensions::BrowserContextKeyedAPI {
 public:
  static extensions::BrowserContextKeyedAPIFactory<AuthPrivateAPI>*
  GetFactoryInstance();

 private:
  friend class extensions::BrowserContextKeyedAPIFactory<AuthPrivateAPI>;

  AuthPrivateAPI() = default;
  ~AuthPrivateAPI() override = default;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "AuthPrivateAPI"; }
  static const bool kServiceIsNULLWhileTesting = true;
  static const bool kServiceRedirectedInIncognito = true;
};

template <>
KeyedService* extensions::BrowserContextKeyedAPIFactory<AuthPrivateAPI>::
    BuildServiceInstanceFor(content::BrowserContext* context) const;

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_AUTH_PRIVATE_AUTH_PRIVATE_API_H_
