// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/test_extension_registrar_delegate.h"

namespace extensions {

TestExtensionRegistrarDelegate::TestExtensionRegistrarDelegate() = default;
TestExtensionRegistrarDelegate::~TestExtensionRegistrarDelegate() = default;

void TestExtensionRegistrarDelegate::PostEnableExtension(
    scoped_refptr<const Extension> extension) {}

void TestExtensionRegistrarDelegate::PostDisableExtension(
    scoped_refptr<const Extension> extension) {}

bool TestExtensionRegistrarDelegate::CanEnableExtension(
    const Extension* extension) {
  return true;
}

bool TestExtensionRegistrarDelegate::CanDisableExtension(
    const Extension* extension) {
  return true;
}

bool TestExtensionRegistrarDelegate::ShouldBlockExtension(
    const ExtensionId& extension_id) {
  return false;
}

}  // namespace extensions
