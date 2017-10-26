// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_FEATURES_FEATURE_CONFIG_H_
#define EXTENSIONS_COMMON_FEATURES_FEATURE_CONFIG_H_

#include <initializer_list>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/optional.h"
#include "components/version_info/version_info.h"
#include "extensions/common/features/feature.h"
#include "extensions/common/features/feature_session_type.h"
#include "extensions/common/manifest.h"

namespace extensions {

enum class SimpleFeatureLocation;

struct SimpleFeatureConfig {
  const char* name;
  std::initializer_list<const char*> blacklist;
  const char* command_line_switch;
  std::initializer_list<const char*> dependencies;
  std::initializer_list<const char*> matches;
  std::initializer_list<const char*> whitelist;
  uint16_t contexts;        // Feature::Context
  int8_t channel;           // version_info::Channel (not bitmask)
  uint8_t extension_types;  // Manifest::Type
  int8_t location;          // SimpleFeature::Location
  int8_t max_manifest_version;
  int8_t min_manifest_version;
  uint8_t platforms;      // Feature::Platform
  uint8_t session_types;  // FeatureSessionType
  bool component_extensions_auto_granted : 1;
  bool internal : 1;
  bool no_parent : 1;
};

struct ComplexFeatureConfig {
  const char* name;
  std::initializer_list<SimpleFeatureConfig> features;
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_FEATURES_FEATURE_CONFIG_H_
