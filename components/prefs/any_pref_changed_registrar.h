// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREFS_ANY_PREF_CHANGED_REGISTRAR_H_
#define COMPONENTS_PREFS_ANY_PREF_CHANGED_REGISTRAR_H_

#include "base/callback.h"
#include "base/macros.h"
#include "components/prefs/base_prefs_export.h"
#include "components/prefs/pref_observer.h"

class PrefService;

// Registers for notification when *any* preference has changed.
//
// THIS CLASS SHOULD BE AVOIDED!
//
// Preferences change frequently and firing many observers for each one is very
// inefficient. If your design requires this, please consider changing your
// design. If it can not be avoided, using this class is better than
// registering for changes on hundreds of different preferences, as long as you
// do very little work for each notification.
class COMPONENTS_PREFS_EXPORT AnyPrefChangedRegistrar : public PrefObserver {
 public:
  using NamedChangeCallback = base::RepeatingCallback<void(const std::string&)>;

  AnyPrefChangedRegistrar();
  ~AnyPrefChangedRegistrar();

  // Only one callback may be registered at a time.
  void Register(PrefService* service, const NamedChangeCallback& obs);
  void Unregister();

 private:
  // PrefObserver:
  void OnPreferenceChanged(PrefService* service,
                           const std::string& pref_name) override;

  PrefService* service_ = nullptr;
  NamedChangeCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(AnyPrefChangedRegistrar);
};

#endif  // COMPONENTS_PREFS_ANY_PREF_CHANGED_REGISTRAR_H_
