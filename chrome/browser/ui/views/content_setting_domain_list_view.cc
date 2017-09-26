// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/content_setting_domain_list_view.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/bullet_view.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"

namespace {

constexpr int kTitleColumnSetId = 0;
constexpr int kDetailsColumnSetId = 1;

void BuildLayout(views::GridLayout* layout) {
  constexpr auto FILL = views::GridLayout::FILL;
  constexpr auto USE_PREF = views::GridLayout::USE_PREF;
  constexpr auto FIXED = views::GridLayout::FIXED;

  views::ColumnSet* title_columns = layout->AddColumnSet(kTitleColumnSetId);
  title_columns->AddColumn(FILL, FILL, 1, USE_PREF, 0, 0);

  views::ColumnSet* details_columns = layout->AddColumnSet(kDetailsColumnSetId);
  int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_UNRELATED_CONTROL_HORIZONTAL);
  details_columns->AddColumn(FILL, FILL, 0, FIXED, width, width);
  details_columns->AddColumn(FILL, FILL, 1, USE_PREF, 0, 0);
}

}  // namespace

ContentSettingDomainListView::ContentSettingDomainListView(
    const base::string16& title,
    const std::set<std::string>& domains) {
  views::GridLayout* layout = views::GridLayout::CreateAndInstall(this);
  BuildLayout(layout);

  auto title_label = std::make_unique<views::Label>(title);
  title_label->SetMultiLine(true);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->StartRow(0, kTitleColumnSetId);
  layout->AddView(title_label.release());

  for (const auto& domain : domains) {
    layout->StartRow(0, kDetailsColumnSetId);

    auto domain_label =
        base::MakeUnique<views::Label>(base::UTF8ToUTF16(domain));
    auto marker = std::make_unique<BulletView>(domain_label->enabled_color());
    domain_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

    layout->AddView(marker.release());
    layout->AddView(domain_label.release());
  }
}

ContentSettingDomainListView::~ContentSettingDomainListView() {}
