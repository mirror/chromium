// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/content_setting_domain_list_view.h"

#include "base/strings/utf_string_conversions.h"
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

// An EntryMarkerView draws a small circle at its center, to visually mark an
// entry from an unordered list.
class EntryMarkerView : public views::View {
 public:
  explicit EntryMarkerView(SkColor color) : color_(color) {}
  ~EntryMarkerView() override {}

  void OnPaint(gfx::Canvas* canvas) override {
    OnPaintBackground(canvas);
    OnPaintBorder(canvas);

    SkScalar radius = std::min(height(), width()) / 8.0;
    gfx::Point center = GetLocalBounds().CenterPoint();

    SkPath path;
    path.addCircle(center.x(), center.y(), radius);

    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kStrokeAndFill_Style);
    flags.setColor(color_);
    flags.setAntiAlias(true);

    canvas->DrawPath(path, flags);
  }

 private:
  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(EntryMarkerView);
};

}  // namespace

ContentSettingDomainListView::ContentSettingDomainListView(
    const base::string16& title,
    const std::set<std::string>& domains) {
  views::GridLayout* layout = views::GridLayout::CreateAndInstall(this);
  BuildLayout(layout);

  auto title_label = base::MakeUnique<views::Label>(title);
  SkColor marker_color = title_label->enabled_color();
  title_label->SetMultiLine(true);
  title_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->StartRow(0, kTitleColumnSetId);
  layout->AddView(title_label.release());

  for (const auto& domain : domains) {
    layout->StartRow(0, kDetailsColumnSetId);

    auto marker = base::MakeUnique<EntryMarkerView>(marker_color);
    auto domain_label =
        base::MakeUnique<views::Label>(base::UTF8ToUTF16(domain));
    domain_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

    layout->AddView(marker.release());
    layout->AddView(domain_label.release());
  }
}

ContentSettingDomainListView::~ContentSettingDomainListView() {}
