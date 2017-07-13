// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_tiles/exploration_section.h"
#include "base/strings/utf_string_conversions.h"

namespace ntp_tiles {
namespace {

base::string16 GetDefaultTitleForSection(SectionType section_type) {
  // TODO(fhorschig): I18n! Then close https://crbug/740908.
  switch (section_type) {
    case SectionType::PERSONALIZED:
      return base::ASCIIToUTF16("Personalized");
    case SectionType::SOCIAL:
      return base::ASCIIToUTF16("Social");
    case SectionType::ENTERTAINMENT:
      return base::ASCIIToUTF16("Entertainment");
    case SectionType::NEWS:
      return base::ASCIIToUTF16("News");
    case SectionType::ECOMMERCE:
      return base::ASCIIToUTF16("Ecommerce");
    case SectionType::TOOLS:
      return base::ASCIIToUTF16("Tools");
    case SectionType::TRAVEL:
      return base::ASCIIToUTF16("Travel");
  }
  NOTREACHED();
  return base::string16();
}
}  // namespace

ExplorationSection::ExplorationSection(SectionType section_type)
    : type(section_type), title(GetDefaultTitleForSection(section_type)) {}

ExplorationSection::ExplorationSection(const ExplorationSection&) = default;

ExplorationSection::~ExplorationSection() {}

bool operator==(const ExplorationSection& a, const ExplorationSection& b) {
  return (a.title == b.title) && (a.type == b.type);
}

bool operator!=(const ExplorationSection& a, const ExplorationSection& b) {
  return !(a == b);
}

}  // namespace ntp_tiles
