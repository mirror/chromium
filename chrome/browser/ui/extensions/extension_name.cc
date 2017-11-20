// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/extension_name.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"

namespace extensions {

base::string16 GetExtensionName(const GURL& url,
                                content::WebContents* web_contents) {
  // On ChromeOS, this function can be called using web_contents from
  // SimpleWebViewDialog::GetWebContents() which always returns null.
  // TODO(crbug.com/680329) Remove the null check and make
  // SimpleWebViewDialog::GetWebContents return the proper web contents instead.
  if (!web_contents || !url.SchemeIs(extensions::kExtensionScheme))
    return base::string16();

  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  extensions::ExtensionRegistry* extension_registry =
      extensions::ExtensionRegistry::Get(browser_context);
  const extensions::Extension* extension =
      extension_registry->enabled_extensions().GetByID(url.host());
  return extension ? base::CollapseWhitespace(
                         base::UTF8ToUTF16(extension->name()), false)
                   : base::string16();
}

}  // namespace extensions
