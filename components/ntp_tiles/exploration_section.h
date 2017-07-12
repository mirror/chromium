// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_TILES_EXPLORATION_SECTION_H_
#define COMPONENTS_NTP_TILES_EXPLORATION_SECTION_H_

#include "base/strings/string16.h"
#include "components/ntp_tiles/section_type.h"
#include "url/gurl.h"

namespace ntp_tiles {

// A section containing multiple exploration tiles shown on the New Tab Page.
struct ExplorationSection {
  SectionType type;
  base::string16 title;

  explicit ExplorationSection(SectionType section_type);
  ExplorationSection(const ExplorationSection&);
  ~ExplorationSection();
};

bool operator==(const ExplorationSection& a, const ExplorationSection& b);
bool operator!=(const ExplorationSection& a, const ExplorationSection& b);

}  // namespace ntp_tiles

#endif  // COMPONENTS_NTP_TILES_EXPLORATION_SECTION_H_
