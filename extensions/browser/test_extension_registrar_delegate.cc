// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/test_extension_registrar_delegate.h"

#include "extensions/common/disable_reason.h"

namespace extensions {

TestExtensionRegistrarDelegate::TestExtensionRegistrarDelegate(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {}

TestExtensionRegistrarDelegate::~TestExtensionRegistrarDelegate() = default;

// ExtensionRegistrar::Delegate:
int TestExtensionRegistrarDelegate::PreAddExtension(const Extension* extension,
                                                    bool is_extension_loaded) {
  // TODO
  LOG(ERROR) << browser_context_;
  return disable_reason::DISABLE_NONE;
}

void TestExtensionRegistrarDelegate::PostEnableExtension(
    scoped_refptr<const Extension> extension) {}

void TestExtensionRegistrarDelegate::PostDisableExtension(
    scoped_refptr<const Extension> extension) {}

void TestExtensionRegistrarDelegate::OnAddedExtensionDisabled(
    const Extension* extension) {}

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

void TestExtensionRegistrarDelegate::LoadExtensionForReload(
    const ExtensionId& extension_id,
    const base::FilePath& path) {
  // TODO: ???
}

}  // namespace extensions
