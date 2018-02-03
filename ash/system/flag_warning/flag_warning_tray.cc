// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/flag_warning/flag_warning_tray.h"

#include <memory>

#include "ash/public/cpp/ash_typography.h"
#include "ash/public/cpp/config.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_container.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

FlagWarningTray::FlagWarningTray(Shelf* shelf) {
  DCHECK(shelf);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  // The flag --mash is a chrome-level switch, so check the config instead.
  const bool is_mash = Shell::GetAshConfig() == Config::MASH;
  if (is_mash) {
    container_ = new TrayContainer(shelf);
    AddChildView(container_);

    button_ = views::MdTextButton::Create(this, base::ASCIIToUTF16("mash"),
                                          CONTEXT_LAUNCHER_BUTTON);
    button_->SetProminent(true);
    button_->SetBgColorOverride(gfx::kGoogleRed700);
    button_->SetMinSize(gfx::Size(0, kTrayItemSize));
    container_->AddChildView(button_);
  }
  SetVisible(is_mash);
}

FlagWarningTray::~FlagWarningTray() = default;

void FlagWarningTray::UpdateAfterShelfAlignmentChange() {
  if (container_)
    container_->UpdateAfterShelfAlignmentChange();
}

void FlagWarningTray::ButtonPressed(views::Button* sender,
                                    const ui::Event& event) {
  DCHECK_EQ(button_, sender);

  // TODO(jamescook): Use NewWindowController to open about:flags. This will
  // require a new mojo interface to chrome.
}

void FlagWarningTray::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  View::GetAccessibleNodeData(node_data);
  node_data->SetName(button_->GetText());
}

}  // namespace ash
