// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREF_H_
#define CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREF_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chrome/browser/extensions/api/settings_private/generated_prefs.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util_enums.h"

namespace base {
class Value;
}

namespace extensions {
namespace api {
namespace settings_private {
struct PrefObject;
}  // namespace settings_private
}  // namespace api

namespace settings_private {

// Base class for generated preference implementation.
class GeneratedPref {
 public:
  virtual ~GeneratedPref();

  virtual std::unique_ptr<api::settings_private::PrefObject> GetPrefObject()
      const = 0;
  virtual SetPrefResult SetPref(const base::Value* value) = 0;
  void AddPrefObserver(const GeneratedPrefs::PrefChangeCallback& observer);

 protected:
  GeneratedPref();

  void NotifyObservers(const std::string& pref_name);

  std::vector<GeneratedPrefs::PrefChangeCallback> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(GeneratedPref);
};

}  // namespace settings_private
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREF_H_
