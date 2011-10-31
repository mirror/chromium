// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_INFO_MAP_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_INFO_MAP_H_
#pragma once

#include <map>
#include <string>

#include "base/basictypes.h"
#include "base/time.h"
#include "base/memory/ref_counted.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_set.h"

class Extension;

// Contains extension data that needs to be accessed on the IO thread. It can
// be created/destroyed on any thread, but all other methods must be called on
// the IO thread.
class ExtensionInfoMap : public base::RefCountedThreadSafe<ExtensionInfoMap> {
 public:
  ExtensionInfoMap();
  ~ExtensionInfoMap();

  const ExtensionSet& extensions() const { return extensions_; }
  const ExtensionSet& disabled_extensions() const {
    return disabled_extensions_;
  }

  // Callback for when new extensions are loaded.
  void AddExtension(const Extension* extension,
                    base::Time install_time,
                    bool incognito_enabled);

  // Callback for when an extension is unloaded.
  void RemoveExtension(const std::string& extension_id,
                       const extension_misc::UnloadedExtensionReason reason);

  // Returns the time the extension was installed, or base::Time() if not found.
  base::Time GetInstallTime(const std::string& extension_id) const;

  // Returns true if the user has allowed this extension to run in incognito
  // mode.
  bool IsIncognitoEnabled(const std::string& extension_id) const;

  // Returns true if the given extension can see events and data from another
  // sub-profile (incognito to original profile, or vice versa).
  bool CanCrossIncognito(const Extension* extension);

  // Record that |extension_id| is running in |process_id|. We normally have
  // this information in  ExtensionProcessManager on the UI thread, but we also
  // sometimes need it on the IO thread. Note that this can be any of
  // (extension, packaged app, hosted app).
  void RegisterExtensionProcess(const std::string& extension_id,
                                int process_id);

  // Remove any record of |extension_id| created with RegisterExtensionProcess.
  // If |extension_id| is unknown, we ignore it.
  void UnregisterExtensionProcess(const std::string& extension_id,
                                  int process_id);

  // Returns true if |extension_id| is running in |process_id|..
  bool IsExtensionInProcess(const std::string& extension_id,
                            int process_id) const;

 private:
  // Extra dynamic data related to an extension.
  struct ExtraData;
  // Map of extension_id to ExtraData.
  typedef std::map<std::string, ExtraData> ExtraDataMap;

  ExtensionSet extensions_;
  ExtensionSet disabled_extensions_;

  // Extra data associated with enabled extensions.
  ExtraDataMap extra_data_;

  typedef std::multimap<std::string, int> ExtensionProcessIDMap;
  ExtensionProcessIDMap extension_process_ids_;
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_INFO_MAP_H_
