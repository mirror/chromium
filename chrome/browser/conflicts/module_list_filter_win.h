// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MODULE_LIST_FILTER_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MODULE_LIST_FILTER_WIN_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/conflicts/proto/module_list.pb.h"
#include "url/gurl.h"

struct ModuleInfoKey;
struct ModuleInfoData;

namespace base {
class FilePath;
}

// This class is used to determine if a module should be blacklisted or
// whitelisted depending on the ModuleList component (See module_list.proto).
class ModuleListFilter {
 public:
  ModuleListFilter();
  ~ModuleListFilter();

  // Initializes the filter with the serialized proto at |module_list_path|.
  // This must be invoked before any calls to IsWhitelisted() and
  // IsBlacklisted().
  bool Initialize(const base::FilePath& module_list_path);

  // Returns true if the module is whitelisted.
  //
  // A whitelisted module should not trigger any warning to the user, nor
  // should it be blocked from loading into the process.
  //
  // Marked virtual to allow mocking.
  virtual bool IsWhitelisted(const ModuleInfoKey& module_key,
                             const ModuleInfoData& module_data) const;

  // Returns true if the module is blacklisted.
  //
  // A blacklisted module can cause instability if allowed into the process.
  //
  // For each blacklisted module, this function also returns a boolean that
  // indicates if the module should be allowed to load and which kind of message
  // should be displayed to the user, if applicable.
  //
  // Marked virtual to allow mocking.
  virtual bool IsBlacklisted(
      const ModuleInfoKey& module_key,
      const ModuleInfoData& module_data,
      bool* allow_load,
      chrome::conflicts::BlacklistMessageType* blacklist_message_type,
      std::string* message_url) const;

 private:
  chrome::conflicts::ModuleList module_list_;

  // Indicates if Initalize() has been called yet.
  bool initialized_ = false;

  DISALLOW_COPY_AND_ASSIGN(ModuleListFilter);
};

#endif  // CHROME_BROWSER_CONFLICTS_MODULE_LIST_FILTER_WIN_H_
