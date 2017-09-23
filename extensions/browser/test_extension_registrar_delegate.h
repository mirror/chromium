// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_TEST_EXTENSION_REGISTRAR_DELEGATE_H_
#define EXTENSIONS_BROWSER_TEST_EXTENSION_REGISTRAR_DELEGATE_H_

#include "extensions/browser/extension_registrar.h"

#include "base/macros.h"

namespace extensions {

class TestExtensionRegistrarDelegate : public ExtensionRegistrar::Delegate {
 public:
  TestExtensionRegistrarDelegate();
  ~TestExtensionRegistrarDelegate() override;

  // ExtensionRegistrar::Delegate:
  void PostEnableExtension(scoped_refptr<const Extension> extension) override;
  void PostDisableExtension(scoped_refptr<const Extension> extension) override;
  bool CanEnableExtension(const Extension* extension) override;
  bool CanDisableExtension(const Extension* extension) override;
  bool ShouldBlockExtension(const ExtensionId& extension_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestExtensionRegistrarDelegate);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_TEST_EXTENSION_REGISTRAR_DELEGATE_H_
