// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/harmony/layout_helper.h"

#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/grid_layout.h"

using views::GridLayout;

namespace {

constexpr float kFixed = 0.f;
constexpr float kStretchy = 1.f;

views::Textfield* AddTextfieldRowImpl(GridLayout* layout,
                                      int column_set_id,
                                      const base::string16& label_text,
                                      bool first) {
  constexpr int kHarmonyTextfieldHeight = 28;
  constexpr int kHarmonyTextfieldVerticalPadding = 12;

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  layout->AddPaddingRow(kFixed,
                        first ? provider->GetDistanceMetric(
                                    DISTANCE_UNRELATED_CONTROL_VERTICAL_LARGE)
                              : kHarmonyTextfieldVerticalPadding);

  layout->StartRow(kFixed, column_set_id, kHarmonyTextfieldHeight);
  views::Label* label = new views::Label(
      label_text, views::style::CONTEXT_LABEL, views::style::STYLE_PRIMARY);

  views::Textfield* textfield = new views::Textfield();
  textfield->SetAccessibleName(label_text);

  // The first column is the margin. It is not skipped automatically because it
  // can host column-spanning Views.
  layout->SkipColumns(1);
  layout->AddView(label);
  layout->AddView(textfield);

  return textfield;
}

}  // namespace

views::ColumnSet* ConfigureTextfieldStack(GridLayout* layout,
                                          int column_set_id) {
  constexpr int kTextfieldStackHorizontalSpacing = 30;
  constexpr int kMinHarmonyTextfieldStart = 144;

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  const int side_padding =
      provider->UseExtraDialogPadding() ? kTextfieldStackHorizontalSpacing : 0;
  const int between_padding =
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_HORIZONTAL);
  const int label_min_width =
      ui::MaterialDesignController::IsSecondaryUiMaterial()
          ? kMinHarmonyTextfieldStart - between_padding
          : 0;

  views::ColumnSet* column_set = layout->AddColumnSet(column_set_id);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, kFixed,
                        GridLayout::USE_PREF, 0, side_padding);
  column_set->AddColumn(provider->GetControlLabelGridAlignment(),
                        GridLayout::CENTER, kFixed, GridLayout::USE_PREF, 0,
                        label_min_width);
  column_set->AddPaddingColumn(kFixed, between_padding);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, kStretchy,
                        GridLayout::USE_PREF, 0, 0);
  column_set->AddPaddingColumn(kFixed, side_padding);
  return column_set;
}

views::Textfield* AddFirstTextfieldRow(GridLayout* layout,
                                       const base::string16& label,
                                       int column_set_id) {
  return AddTextfieldRowImpl(layout, column_set_id, label, true);
}

views::Textfield* AddTextfieldRow(GridLayout* layout,
                                  const base::string16& label,
                                  int column_set_id) {
  return AddTextfieldRowImpl(layout, column_set_id, label, false);
}
