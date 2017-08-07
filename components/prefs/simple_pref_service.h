// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This provides a way to access the application's current preferences.

// Chromium settings and storage represent user-selected preferences and
// information and MUST not be extracted, overwritten or modified except
// through Chromium defined APIs.

#ifndef COMPONENTS_PREFS_SIMPLE_PREF_SERVICE_H_
#define COMPONENTS_PREFS_SIMPLE_PREF_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "components/prefs/base_prefs_export.h"
#include "components/prefs/persistent_pref_store.h"

class COMPONENTS_PREFS_EXPORT SimplePrefService {
 public:
  SimplePrefService();
  virtual ~SimplePrefService();

  virtual int64_t GetInt64(const std::string& path) const;
  virtual void SetInt64(const std::string& path, int64_t value);

  virtual const base::ListValue* GetList(const std::string& path) const;

  virtual void ClearPref(const std::string& path);
  virtual bool HasPrefPath(const std::string& path) const;

  virtual void Set(const std::string& path, const base::Value& value);

  virtual std::string GetString(const std::string& path) const;
  virtual void SetString(const std::string& path, const std::string& value);

 private:
  std::string compressed_seed;
  std::string seed_signature;
  std::string country_code;
  int64_t seed_date; //TODO: add trailing underscores
  int64_t fetch_time;

  const std::string kVariationsCountry = "variations_country";
  const std::string kVariationsCompressedSeed = "variations_compressed_seed";
  const std::string kVariationsSeedSignature = "variations_seed_signature";
  const std::string kVariationsSeedDate = "variations_seed_date";
  const std::string kVariationsLastFetchTime = "variations_last_fetch_time";

  DISALLOW_COPY_AND_ASSIGN(SimplePrefService);
};

#endif  // COMPONENTS_PREFS_SIMPLE_PREF_SERVICE_H_
