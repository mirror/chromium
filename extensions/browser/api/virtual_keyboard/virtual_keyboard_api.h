// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_API_H_
#define EXTENSIONS_BROWSER_API_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_API_H_

#include "base/macros.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class VirtualKeyboardRestrictFeaturesFunction
    : public UIThreadExtensionFunction {
 public:
  VirtualKeyboardRestrictFeaturesFunction();

  DECLARE_EXTENSION_FUNCTION("virtualKeyboard.restrictFeatures",
                             VIRTUALKEYBOARD_RESTRICTFEATURES);

 protected:
  ~VirtualKeyboardRestrictFeaturesFunction() override = default;
  // UIThreadExtensionFunction override:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VirtualKeyboardRestrictFeaturesFunction);
};

class VirtualKeyboardDelegate;

class VirtualKeyboardAPI : public BrowserContextKeyedAPI {
 public:
  explicit VirtualKeyboardAPI(content::BrowserContext* context);
  ~VirtualKeyboardAPI() override;

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<VirtualKeyboardAPI>*
  GetFactoryInstance();

  VirtualKeyboardDelegate* delegate() { return delegate_.get(); }

 private:
  friend class BrowserContextKeyedAPIFactory<VirtualKeyboardAPI>;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "VirtualKeyboardAPI"; }

  // Require accces to delegate while incognito or during login.
  static const bool kServiceHasOwnInstanceInIncognito = true;

  std::unique_ptr<VirtualKeyboardDelegate> delegate_;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_VIRTUAL_KEYBOARD_VIRTUAL_KEYBOARD_API_H_
