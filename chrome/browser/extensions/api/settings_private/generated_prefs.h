// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREFS_H_
#define CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREFS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util_enums.h"

class Profile;

namespace base {
class Value;
}

namespace extensions {
namespace api {
namespace settings_private {
struct PrefObject;
}  // namespace settings_private
}  // namespace api

class GeneratedPrefImplBase;

class GeneratedPrefs {
 public:
  // Pref changed callback.
  // Input: preference name.
  using PrefChangeObserver = base::RepeatingCallback<void(const std::string&)>;

  // Preference name to implementation map.
  using PrefsMap =
      std::unordered_map<std::string, std::unique_ptr<GeneratedPrefImplBase>>;

  explicit GeneratedPrefs(Profile* profile);
  ~GeneratedPrefs();

  // Returns true if preference is supported.
  bool IsGenerated(const std::string& pref_name) const;

  // Returns fully populated PrefObject or nullptr if not supported.
  std::unique_ptr<api::settings_private::PrefObject> GetPref(
      const std::string& pref_name) const;

  // Updates preference value.
  SetPrefResult SetPref(const std::string& pref_name, const base::Value* value);

  // Adds callback that will be called when given preference is changed.
  // Will trigger fatal error if preference is not generated.
  void AddPrefObserver(const std::string& pref_name,
                       const PrefChangeObserver& observer);

 private:
  // Returns preference implementation
  GeneratedPrefImplBase* FindPrefImpl(const std::string& pref_name) const;

  // Known preference map.
  PrefsMap prefs_;

  DISALLOW_COPY_AND_ASSIGN(GeneratedPrefs);
};

// Base class for generated preference implementation.
class GeneratedPrefImplBase {
 public:
  virtual ~GeneratedPrefImplBase();

  virtual std::unique_ptr<api::settings_private::PrefObject> GetPrefObject()
      const = 0;
  virtual SetPrefResult SetPref(const base::Value* value) = 0;
  void AddPrefObserver(const GeneratedPrefs::PrefChangeObserver& observer);

 protected:
  GeneratedPrefImplBase();

  void NotifyObservers(const std::string& pref_name);

  std::vector<GeneratedPrefs::PrefChangeObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(GeneratedPrefImplBase);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREFS_H_
