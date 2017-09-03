// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FONT_PREF_CHANGE_NOTIFIER_H_
#define CHROME_BROWSER_FONT_PREF_CHANGE_NOTIFIER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/prefs/any_pref_changed_registrar.h"

// There are more than 1000 font prefs and several subsystems need to be
// notified when a font changes. Registering for 1000 font change notifications
// is inefficient and slows down other pref notifications, and having each font
// subsystem registering "any preference changed" observers is also
// inefficient.
//
// This class registers for "any prefernece changed" notifications, filters
// for font pref changes only, and then issues notifications to registered
// observers. There should be one FontPrefChangeNotifier per Profile.
class FontPrefChangeNotifier {
 public:
  // The parameter is the full name of the font pref that changed.
  using Callback = base::RepeatingCallback<void(const std::string&)>;

  // Instantiate this subclass to scope an observer notification. The registrar
  // can have only one callback.
  class Registrar {
   public:
    Registrar();
    ~Registrar();

    bool is_registered() const { return !!notifier_; }

    // Start watching for changes.
    void Register(FontPrefChangeNotifier* notifier,
                  FontPrefChangeNotifier::Callback cb);

    // Optional way to unregister before the Registrar object goes out of
    // scope. The object must currently be registered.
    void Unregister();

   private:
    friend FontPrefChangeNotifier;

    FontPrefChangeNotifier* notifier_ = nullptr;
    FontPrefChangeNotifier::Callback callback_;

    DISALLOW_COPY_AND_ASSIGN(Registrar);
  };

  // The pref service must outlive this class.
  FontPrefChangeNotifier(PrefService* pref_service);
  ~FontPrefChangeNotifier();

 private:
  friend Registrar;

  void AddRegistrar(Registrar* registrar);
  void RemoveRegistrar(Registrar* registrar);

  void AnyPrefChanged(const std::string& pref_name);

  PrefService* pref_service_;  // Non-owning.
  bool has_registered_ = false;
  AnyPrefChangedRegistrar any_pref_changed_;

  // Non-owning pointers to the Registrars that have registered themselves
  // with us. We expect few registrars.
  base::ObserverList<Registrar> registrars_;

  DISALLOW_COPY_AND_ASSIGN(FontPrefChangeNotifier);
};

#endif  // CHROME_BROWSER_FONT_PREF_CHANGE_NOTIFIER_H_
