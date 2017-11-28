// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/device_chooser_content_view.h"

#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/controls/table/table_view.h"
#include "ui/views/controls/throbber.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kThrobberDiameter = 24;

constexpr int kAdapterOffHelpLinkPadding = 5;

// The lookup table for signal strength level image.
constexpr int kSignalStrengthLevelImageIds[5] = {
    IDR_SIGNAL_0_BAR, IDR_SIGNAL_1_BAR, IDR_SIGNAL_2_BAR, IDR_SIGNAL_3_BAR,
    IDR_SIGNAL_4_BAR};

constexpr int kHelpButtonTag = 1;
constexpr int kReScanButtonTag = 2;

}  // namespace

DeviceChooserContentView::DeviceChooserContentView(
    views::TableViewObserver* table_view_observer,
    std::unique_ptr<ChooserController> chooser_controller)
    : chooser_controller_(std::move(chooser_controller)) {
  chooser_controller_->set_view(this);
  std::vector<ui::TableColumn> table_columns;
  table_columns.push_back(ui::TableColumn());
  table_view_ = new views::TableView(
      this, table_columns,
      chooser_controller_->ShouldShowIconBeforeText() ? views::ICON_AND_TEXT
                                                      : views::TEXT_ONLY,
      !chooser_controller_->AllowMultipleSelection() /* single_selection */);
  table_view_->set_select_on_remove(false);
  table_view_->set_observer(table_view_observer);
  table_view_->SetEnabled(chooser_controller_->NumOptions() > 0);

  table_parent_ = table_view_->CreateParentIfNecessary();
  AddChildView(table_parent_);

  throbber_ = new views::Throbber();
  throbber_->SetVisible(false);
  AddChildView(throbber_);

  base::string16 link_text = l10n_util::GetStringUTF16(
      IDS_BLUETOOTH_DEVICE_CHOOSER_TURN_ON_BLUETOOTH_LINK_TEXT);
  size_t offset = 0;
  base::string16 text = l10n_util::GetStringFUTF16(
      IDS_BLUETOOTH_DEVICE_CHOOSER_TURN_ADAPTER_OFF, link_text, &offset);
  turn_adapter_off_help_ = new views::StyledLabel(text, this);
  turn_adapter_off_help_->AddStyleRange(
      gfx::Range(0, link_text.size()),
      views::StyledLabel::RangeStyleInfo::CreateForLink());
  turn_adapter_off_help_->SetVisible(false);
  AddChildView(turn_adapter_off_help_);
}

DeviceChooserContentView::~DeviceChooserContentView() {
  chooser_controller_->set_view(nullptr);
  table_view_->set_observer(nullptr);
  table_view_->SetModel(nullptr);
}

gfx::Size DeviceChooserContentView::GetMinimumSize() const {
  // Let the dialog shrink when its parent is smaller than the preferred size.
  return gfx::Size();
}

void DeviceChooserContentView::Layout() {
  gfx::Rect rect(GetContentsBounds());
  table_parent_->SetBoundsRect(rect);
  // Set the throbber in the center of the chooser.
  throbber_->SetBounds((rect.width() - kThrobberDiameter) / 2,
                       (rect.height() - kThrobberDiameter) / 2,
                       kThrobberDiameter, kThrobberDiameter);
  // Set the adapter off message in the center of the chooser.
  // The adapter off message will only be shown when the adapter is off,
  // and in that case, the system won't be able to scan for devices, so
  // the throbber won't be shown at the same time.
  turn_adapter_off_help_->SetPosition(
      gfx::Point((rect.width() - turn_adapter_off_help_->width()) / 2,
                 (rect.height() - turn_adapter_off_help_->height()) / 2));
  turn_adapter_off_help_->SizeToFit(rect.width() -
                                    2 * kAdapterOffHelpLinkPadding);
  views::View::Layout();
}

gfx::Size DeviceChooserContentView::CalculatePreferredSize() const {
  return gfx::Size(402, 320);
}

int DeviceChooserContentView::RowCount() {
  // When there are no devices, the table contains a message saying there
  // are no devices, so the number of rows is always at least 1.
  return std::max(base::checked_cast<int>(chooser_controller_->NumOptions()),
                  1);
}

base::string16 DeviceChooserContentView::GetText(int row, int column_id) {
  int num_options = base::checked_cast<int>(chooser_controller_->NumOptions());
  if (num_options == 0) {
    DCHECK_EQ(0, row);
    return chooser_controller_->GetNoOptionsText();
  }

  DCHECK_GE(row, 0);
  DCHECK_LT(row, num_options);
  base::string16 text =
      chooser_controller_->GetOption(static_cast<size_t>(row));
  return chooser_controller_->IsPaired(row)
             ? l10n_util::GetStringFUTF16(
                   IDS_DEVICE_CHOOSER_DEVICE_NAME_AND_PAIRED_STATUS_TEXT, text)
             : text;
}

void DeviceChooserContentView::SetObserver(ui::TableModelObserver* observer) {}

gfx::ImageSkia DeviceChooserContentView::GetIcon(int row) {
  DCHECK(chooser_controller_->ShouldShowIconBeforeText());

  size_t num_options = chooser_controller_->NumOptions();
  if (num_options == 0) {
    DCHECK_EQ(0, row);
    return gfx::ImageSkia();
  }

  DCHECK_GE(row, 0);
  DCHECK_LT(row, base::checked_cast<int>(num_options));

  if (chooser_controller_->IsConnected(row))
    return gfx::CreateVectorIcon(vector_icons::kBluetoothConnectedIcon,
                                 gfx::kChromeIconGrey);

  int level = chooser_controller_->GetSignalStrengthLevel(row);

  if (level == -1)
    return gfx::ImageSkia();

  DCHECK_GE(level, 0);
  DCHECK_LT(level, static_cast<int>(arraysize(kSignalStrengthLevelImageIds)));

  return *ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      kSignalStrengthLevelImageIds[level]);
}

void DeviceChooserContentView::OnOptionsInitialized() {
  table_view_->OnModelChanged();
  UpdateTableView();
}

void DeviceChooserContentView::OnOptionAdded(size_t index) {
  table_view_->OnItemsAdded(base::checked_cast<int>(index), 1);
  UpdateTableView();
  table_view_->SetVisible(true);
  throbber_->SetVisible(false);
  throbber_->Stop();
}

void DeviceChooserContentView::OnOptionRemoved(size_t index) {
  table_view_->OnItemsRemoved(base::checked_cast<int>(index), 1);
  UpdateTableView();
}

void DeviceChooserContentView::OnOptionUpdated(size_t index) {
  table_view_->OnItemsChanged(base::checked_cast<int>(index), 1);
  UpdateTableView();
}

void DeviceChooserContentView::OnAdapterEnabledChanged(bool enabled) {
  // No row is selected since the adapter status has changed.
  // This will also disable the OK button if it was enabled because
  // of a previously selected row.
  table_view_->Select(-1);
  UpdateTableView();
  table_view_->SetVisible(enabled);
  turn_adapter_off_help_->SetVisible(!enabled);

  throbber_->Stop();
  throbber_->SetVisible(false);

  SetReScanButtonEnabled(enabled);
}

void DeviceChooserContentView::OnRefreshStateChanged(bool refreshing) {
  if (refreshing) {
    // No row is selected since the chooser is refreshing. This will also
    // disable the OK button if it was enabled because of a previously
    // selected row.
    table_view_->Select(-1);
    UpdateTableView();
  }

  // When refreshing and no option available yet, hide |table_view_| and show
  // |throbber_|. Otherwise show |table_view_| and hide |throbber_|.
  const bool throbber_visible =
      refreshing && (chooser_controller_->NumOptions() == 0);
  table_view_->SetVisible(!throbber_visible);
  throbber_->SetVisible(throbber_visible);
  if (throbber_visible)
    throbber_->Start();
  else
    throbber_->Stop();

  SetReScanButtonEnabled(!refreshing);
}

void DeviceChooserContentView::StyledLabelLinkClicked(views::StyledLabel* label,
                                                      const gfx::Range& range,
                                                      int event_flags) {
  DCHECK_EQ(turn_adapter_off_help_, label);
  chooser_controller_->OpenAdapterOffHelpUrl();
}

void DeviceChooserContentView::ButtonPressed(views::Button* sender,
                                             const ui::Event& event) {
  if (sender->tag() == kHelpButtonTag) {
    chooser_controller_->OpenHelpCenterUrl();
  } else if (sender->tag() == kReScanButtonTag) {
    chooser_controller_->RefreshOptions();
  } else {
    NOTREACHED();
  }
}

base::string16 DeviceChooserContentView::GetWindowTitle() const {
  return chooser_controller_->GetTitle();
}

std::unique_ptr<views::View> DeviceChooserContentView::CreateExtraView() {
  std::vector<views::View*> views;
  if (chooser_controller_->ShouldShowHelpButton()) {
    views::ImageButton* help_button = views::CreateVectorImageButton(this);
    views::SetImageFromVectorIcon(help_button, vector_icons::kHelpOutlineIcon);
    help_button->SetFocusForPlatform();
    help_button->SetTooltipText(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
    help_button->set_tag(kHelpButtonTag);
    views.push_back(help_button);
  }
  if (chooser_controller_->ShouldShowReScanButton()) {
    re_scan_button_ = views::MdTextButton::CreateSecondaryUiButton(
        this, l10n_util::GetStringUTF16(IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN));
    re_scan_button_->SetFocusForPlatform();
    re_scan_button_->SetTooltipText(l10n_util::GetStringUTF16(
        IDS_BLUETOOTH_DEVICE_CHOOSER_RE_SCAN_TOOLTIP));
    re_scan_button_->set_tag(kReScanButtonTag);
    // Ensures that the focus will always cycle to the cancel button. Otherwise
    // if the table is selected it will cycle to the help button, which isn't
    // very useful.
    re_scan_button_->set_request_focus_on_press(true);
    views.push_back(re_scan_button_);
  }
  if (views.size() == 1)
    return std::unique_ptr<views::View>(views.front());
  auto container = std::make_unique<views::View>();
  container->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kHorizontal, gfx::Insets(),
                           ChromeLayoutProvider::Get()->GetDistanceMetric(
                               views::DISTANCE_RELATED_CONTROL_HORIZONTAL)));
  for (auto* view : views)
    container->AddChildView(view);
  return container;
}

base::string16 DeviceChooserContentView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return button == ui::DIALOG_BUTTON_OK
             ? chooser_controller_->GetOkButtonLabel()
             : l10n_util::GetStringUTF16(IDS_DEVICE_CHOOSER_CANCEL_BUTTON_TEXT);
}

bool DeviceChooserContentView::IsDialogButtonEnabled(
    ui::DialogButton button) const {
  return button != ui::DIALOG_BUTTON_OK ||
         !table_view_->selection_model().empty();
}

void DeviceChooserContentView::Accept() {
  std::vector<size_t> indices(
      table_view_->selection_model().selected_indices().begin(),
      table_view_->selection_model().selected_indices().end());
  chooser_controller_->Select(indices);
}

void DeviceChooserContentView::Cancel() {
  chooser_controller_->Cancel();
}

void DeviceChooserContentView::Close() {
  chooser_controller_->Close();
}

void DeviceChooserContentView::UpdateTableView() {
  if (chooser_controller_->NumOptions() == 0) {
    table_view_->OnModelChanged();
    table_view_->SetEnabled(false);
  } else {
    table_view_->SetEnabled(true);
  }
}

void DeviceChooserContentView::SetReScanButtonEnabled(bool enabled) {
  if (re_scan_button_)
    re_scan_button_->SetEnabled(enabled);
}
