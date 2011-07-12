// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/live_sync/live_preferences_sync_test.h"

#include "base/values.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/prefs/scoped_user_pref_update.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"

LivePreferencesSyncTest::LivePreferencesSyncTest(TestType test_type)
    : LiveSyncTest(test_type),
      use_verifier_prefs_(true) {}

LivePreferencesSyncTest::~LivePreferencesSyncTest() {}

PrefService* LivePreferencesSyncTest::GetPrefs(int index) {
  return GetProfile(index)->GetPrefs();
}

PrefService* LivePreferencesSyncTest::GetVerifierPrefs() {
  return verifier()->GetPrefs();
}

void LivePreferencesSyncTest::ChangeBooleanPref(int index,
                                                const char* pref_name) {
  bool new_value = !GetPrefs(index)->GetBoolean(pref_name);
  GetPrefs(index)->SetBoolean(pref_name, new_value);
  if (use_verifier_prefs_)
    GetVerifierPrefs()->SetBoolean(pref_name, new_value);
}

void LivePreferencesSyncTest::ChangeIntegerPref(int index,
                                                const char* pref_name,
                                                int new_value) {
  GetPrefs(index)->SetInteger(pref_name, new_value);
  if (use_verifier_prefs_)
    GetVerifierPrefs()->SetInteger(pref_name, new_value);
}

void LivePreferencesSyncTest::ChangeDoublePref(int index,
                                               const char* pref_name,
                                               double new_value) {
  GetPrefs(index)->SetDouble(pref_name, new_value);
  if (use_verifier_prefs_)
    GetVerifierPrefs()->SetDouble(pref_name, new_value);
}

void LivePreferencesSyncTest::ChangeStringPref(int index,
                                               const char* pref_name,
                                               const std::string& new_value) {
  GetPrefs(index)->SetString(pref_name, new_value);
  if (use_verifier_prefs_)
    GetVerifierPrefs()->SetString(pref_name, new_value);
}

void LivePreferencesSyncTest::AppendStringPref(
    int index,
    const char* pref_name,
    const std::string& append_value) {
  ChangeStringPref(index, pref_name,
                   GetPrefs(index)->GetString(pref_name) + append_value);
}

void LivePreferencesSyncTest::ChangeFilePathPref(int index,
                                               const char* pref_name,
                                               const FilePath& new_value) {
  GetPrefs(index)->SetFilePath(pref_name, new_value);
  if (use_verifier_prefs_)
    GetVerifierPrefs()->SetFilePath(pref_name, new_value);
}

void LivePreferencesSyncTest::ChangeListPref(int index,
                                             const char* pref_name,
                                             const ListValue& new_value) {
  {
    ListPrefUpdate update(GetPrefs(index), pref_name);
    ListValue* list = update.Get();
    for (ListValue::const_iterator it = new_value.begin();
         it != new_value.end();
         ++it) {
      list->Append((*it)->DeepCopy());
    }
  }

  if (use_verifier_prefs_) {
    ListPrefUpdate update_verifier(GetVerifierPrefs(), pref_name);
    ListValue* list_verifier = update_verifier.Get();
    for (ListValue::const_iterator it = new_value.begin();
         it != new_value.end();
         ++it) {
      list_verifier->Append((*it)->DeepCopy());
    }
  }
}

bool LivePreferencesSyncTest::BooleanPrefMatches(const char* pref_name) {
  bool reference_value;
  if (use_verifier_prefs_) {
    reference_value = GetVerifierPrefs()->GetBoolean(pref_name);
  } else {
    reference_value = GetPrefs(0)->GetBoolean(pref_name);
  }
  for (int i = 0; i < num_clients(); ++i) {
    if (reference_value != GetPrefs(i)->GetBoolean(pref_name)) {
      LOG(ERROR) << "Boolean preference " << pref_name << " mismatched in"
                 << "profile " << i << ".";
      return false;
    }
  }
  return true;
}

bool LivePreferencesSyncTest::IntegerPrefMatches(const char* pref_name) {
  int reference_value;
  if (use_verifier_prefs_) {
    reference_value = GetVerifierPrefs()->GetInteger(pref_name);
  } else {
    reference_value = GetPrefs(0)->GetInteger(pref_name);
  }
  for (int i = 0; i < num_clients(); ++i) {
    if (reference_value != GetPrefs(i)->GetInteger(pref_name)) {
      LOG(ERROR) << "Integer preference " << pref_name << " mismatched in"
                 << "profile " << i << ".";
      return false;
    }
  }
  return true;
}

bool LivePreferencesSyncTest::DoublePrefMatches(const char* pref_name) {
  double reference_value;
  if (use_verifier_prefs_) {
    reference_value = GetVerifierPrefs()->GetDouble(pref_name);
  } else {
    reference_value = GetPrefs(0)->GetDouble(pref_name);
  }
  for (int i = 0; i < num_clients(); ++i) {
    if (reference_value != GetPrefs(i)->GetDouble(pref_name)) {
      LOG(ERROR) << "Double preference " << pref_name << " mismatched in"
                 << "profile " << i << ".";
      return false;
    }
  }
  return true;
}

bool LivePreferencesSyncTest::StringPrefMatches(const char* pref_name) {
  std::string reference_value;
  if (use_verifier_prefs_) {
    reference_value = GetVerifierPrefs()->GetString(pref_name);
  } else {
    reference_value = GetPrefs(0)->GetString(pref_name);
  }
  for (int i = 0; i < num_clients(); ++i) {
    if (reference_value != GetPrefs(i)->GetString(pref_name)) {
      LOG(ERROR) << "String preference " << pref_name << " mismatched in"
                 << "profile " << i << ".";
      return false;
    }
  }
  return true;
}

bool LivePreferencesSyncTest::FilePathPrefMatches(const char* pref_name) {
  FilePath reference_value;
  if (use_verifier_prefs_) {
    reference_value = GetVerifierPrefs()->GetFilePath(pref_name);
  } else {
    reference_value = GetPrefs(0)->GetFilePath(pref_name);
  }
  for (int i = 0; i < num_clients(); ++i) {
    if (reference_value != GetPrefs(i)->GetFilePath(pref_name)) {
      LOG(ERROR) << "FilePath preference " << pref_name << " mismatched in"
                 << "profile " << i << ".";
      return false;
    }
  }
  return true;
}

bool LivePreferencesSyncTest::ListPrefMatches(const char* pref_name) {
  const ListValue* reference_value;
  if (use_verifier_prefs_) {
    reference_value = GetVerifierPrefs()->GetList(pref_name);
  } else {
    reference_value = GetPrefs(0)->GetList(pref_name);
  }
  for (int i = 0; i < num_clients(); ++i) {
    if (!reference_value->Equals(GetPrefs(i)->GetList(pref_name))) {
      LOG(ERROR) << "List preference " << pref_name << " mismatched in"
                 << "profile " << i << ".";
      return false;
    }
  }
  return true;
}

void LivePreferencesSyncTest::DisableVerifier() {
  use_verifier_prefs_ = false;
}

bool LivePreferencesSyncTest::EnableEncryption(int index) {
  return GetClient(index)->EnableEncryptionForType(syncable::PREFERENCES);
}

bool LivePreferencesSyncTest::IsEncrypted(int index) {
  return GetClient(index)->IsTypeEncrypted(syncable::PREFERENCES);
}
