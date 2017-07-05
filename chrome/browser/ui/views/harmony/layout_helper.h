// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_HARMONY_LAYOUT_HELPER_H_
#define CHROME_BROWSER_UI_VIEWS_HARMONY_LAYOUT_HELPER_H_

#include "base/strings/string16.h"

namespace views {
class ColumnSet;
class GridLayout;
class Textfield;
}  // namespace views

// Configure a three-column ColumnSet for the standard layout with columns:
// {Label, Padding, Textfield}. More columns can be added after.
views::ColumnSet* ConfigureTextfieldStack(views::GridLayout* layout,
                                          int column_set_id);

// The returned Textfield will be owned by the View hosting |layout|.
views::Textfield* AddFirstTextfieldRow(views::GridLayout* layout,
                                       const base::string16& label,
                                       int column_set_id);
views::Textfield* AddTextfieldRow(views::GridLayout* layout,
                                  const base::string16& label,
                                  int column_set_id);

#endif  // CHROME_BROWSER_UI_VIEWS_HARMONY_LAYOUT_HELPER_H_
