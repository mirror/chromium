// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This provides a way to access the application's current preferences.

// Chromium settings and storage represent user-selected preferences and
// information and MUST not be extracted, overwritten or modified except
// through Chromium defined APIs.
//
#include "components/prefs/simple_pref_service.h"

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

SimplePrefService::SimplePrefService() {
}

SimplePrefService::~SimplePrefService() {
}

int64_t SimplePrefService::GetInt64(const std::string& path) const {
  if (path == kVariationsSeedDate) {
    return seed_date;
  } else if (path == kVariationsLastFetchTime) {
    return fetch_time;
  }
  return 0;
}

void SimplePrefService::SetInt64(const std::string& path, int64_t value) {
  if (path == kVariationsSeedDate) {
    seed_date = value;
  } else if (path == kVariationsLastFetchTime) {
    fetch_time = value;
  }
}

const base::ListValue* SimplePrefService::GetList(const std::string& path) const {
  return new base::ListValue();
}

void SimplePrefService::ClearPref(const std::string& path) {
}

bool SimplePrefService::HasPrefPath(const std::string& path) const {
  return true;
}

void SimplePrefService::Set(const std::string& path, const base::Value& value) {
}

std::string SimplePrefService::GetString(const std::string& path) const {
  if (path == kVariationsSeedSignature)  {
    return seed_signature;
  } else if (path == kVariationsCountry) {
    return country_code;
  } else if (path == kVariationsCompressedSeed) {
    return compressed_seed;
  }

  return "";
}

void SimplePrefService::SetString(const std::string& path, const std::string& value) {
  if (path == kVariationsSeedSignature) {
    seed_signature = value;
  } else if (path == kVariationsCountry) {
    country_code = value;
  } else if (path == kVariationsCompressedSeed) {
    compressed_seed = value;
  }
}
