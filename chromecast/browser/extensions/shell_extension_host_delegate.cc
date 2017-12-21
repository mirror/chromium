// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/shell_extension_host_delegate.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "chromecast/browser/extensions/shell_extension_web_contents_observer.h"
#include "extensions/browser/serial_extension_host_queue.h"

namespace extensions {

ShellExtensionHostDelegate::ShellExtensionHostDelegate() {}

ShellExtensionHostDelegate::~ShellExtensionHostDelegate() {}

void ShellExtensionHostDelegate::OnExtensionHostCreated(
    content::WebContents* web_contents) {
  ShellExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

void ShellExtensionHostDelegate::OnRenderViewCreatedForBackgroundPage(
    ExtensionHost* host) {}

content::JavaScriptDialogManager*
ShellExtensionHostDelegate::GetJavaScriptDialogManager() {
  NOTREACHED();
  return NULL;
}

void ShellExtensionHostDelegate::CreateTab(content::WebContents* web_contents,
                                           const std::string& extension_id,
                                           WindowOpenDisposition disposition,
                                           const gfx::Rect& initial_rect,
                                           bool user_gesture) {
  NOTREACHED();
}

void ShellExtensionHostDelegate::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback,
    const Extension* extension) {}

bool ShellExtensionHostDelegate::CheckMediaAccessPermission(
    content::WebContents* web_contents,
    const GURL& security_origin,
    content::MediaStreamType type,
    const Extension* extension) {
  return true;
}

static base::LazyInstance<SerialExtensionHostQueue>::DestructorAtExit g_queue =
    LAZY_INSTANCE_INITIALIZER;

ExtensionHostQueue* ShellExtensionHostDelegate::GetExtensionHostQueue() const {
  return g_queue.Pointer();
}

}  // namespace extensions
