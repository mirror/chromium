// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGING_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGING_DELEGATE_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace extensions {
class Extension;

// Helper class for Chrome-specific features of the extension messaging API.
// TODO(michaelpg): Make this an actual delegate and move the declaration to a
// common location.
class MessagingDelegate {
 public:
  enum PolicyPermission {
    DISALLOW,           // The host is not allowed.
    ALLOW_SYSTEM_ONLY,  // Allowed only when installed on system level.
    ALLOW_ALL,          // Allowed when installed on system or user level.
  };

  static PolicyPermission IsNativeMessagingHostAllowed(
      content::BrowserContext* browser_context,
      const std::string& native_host_name);

  // If web_contents is a tab, returns a dictionary representing its tab.
  static std::unique_ptr<base::DictionaryValue> MaybeGetTabInfo(
      content::WebContents* web_contents);

  static bool IsTab(content::WebContents* source_contents);

  // Passes true to the provided callback if |url| is allowed to connect from
  // this profile, false otherwise. If unknown, the user will be prompted before
  // an answer is returned.
  static void QueryIncognitoConnectability(
      content::BrowserContext* context,
      const Extension* extension,
      content::WebContents* web_contents,
      const GURL& url,
      const base::Callback<void(bool)>& callback);

 private:
  DISALLOW_COPY_AND_ASSIGN(MessagingDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGING_DELEGATE_H_
