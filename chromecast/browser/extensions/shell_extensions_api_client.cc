// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/shell_extensions_api_client.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "chromecast/browser/extensions/shell_extension_web_contents_observer.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/messaging/messaging_delegate.h"
#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_delegate.h"

namespace extensions {

ShellExtensionsAPIClient::ShellExtensionsAPIClient() {}

ShellExtensionsAPIClient::~ShellExtensionsAPIClient() {}

void ShellExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
  ShellExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

MessagingDelegate* ShellExtensionsAPIClient::GetMessagingDelegate() {
  // The default implementation does nothing, which is fine.
  if (!messaging_delegate_)
    messaging_delegate_ = std::make_unique<MessagingDelegate>();
  return messaging_delegate_.get();
}

}  // namespace extensions
