// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_TILES_TILE_NAME_SOURCE_H_
#define COMPONENTS_NTP_TILES_TILE_NAME_SOURCE_H_

namespace ntp_tiles {

// The source where the name of an NTP tile originates from.
//
// These values must stay in sync with the NTPTileNameSource enum in
// histograms.xml.
//
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.suggestions
enum TileNameSource {
  // The name might be invalid, aggregated, user-set, extracted from history,
  // not loaded or simply not known.
  UNKNOWN = 0,

  // The site title, extracted from the title tag.
  TITLE = 1,

  // The site's manifest contained a usable name attribute.
  MANIFEST = 2,

  // The site provided a meta tag with OpenGraph's site_name property.
  META_TAG = 3,

  // The maximum tile name source value that gets recorded in UMA.
  LAST_RECORDED_NAME_SOURCE = META_TAG
};

}  // namespace ntp_tiles

#endif  // COMPONENTS_NTP_TILES_TILE_NAME_SOURCE_H_
