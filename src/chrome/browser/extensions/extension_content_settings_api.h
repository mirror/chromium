// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_CONTENT_SETTINGS_API_H__
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_CONTENT_SETTINGS_API_H__
#pragma once

#include "chrome/browser/extensions/extension_function.h"

namespace webkit {
namespace npapi {

class PluginList;

}
}

class ClearContentSettingsFunction : public SyncExtensionFunction {
 public:
  virtual bool RunImpl();
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.contentSettings.clear")
};

class GetContentSettingFunction : public SyncExtensionFunction {
 public:
  virtual bool RunImpl();
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.contentSettings.get")
};

class SetContentSettingFunction : public SyncExtensionFunction {
 public:
  virtual bool RunImpl();
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.contentSettings.set")
};

class GetResourceIdentifiersFunction : public AsyncExtensionFunction {
 public:
  virtual bool RunImpl();
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.contentSettings.getResourceIdentifiers")

 private:
  FRIEND_TEST_ALL_PREFIXES(ExtensionApiTest,
                           ContentSettingsGetResourceIdentifiers);

  void GetPluginsOnFileThread();

  // Used to override the global plugin list in tests.
  static void SetPluginListForTesting(webkit::npapi::PluginList* plugin_list);
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_CONTENT_SETTINGS_API_H__
