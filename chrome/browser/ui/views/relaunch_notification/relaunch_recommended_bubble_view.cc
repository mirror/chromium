// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/relaunch_notification/relaunch_recommended_bubble_view.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/app_menu_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/widget/widget.h"

// static
views::Widget* RelaunchRecommendedBubbleView::ShowBubble(
    Browser* browser,
    base::TimeTicks detection_time) {
  DCHECK(browser);

  // Anchor the popup to the browser's app menu.
  views::View* app_menu_button = BrowserView::GetBrowserViewForBrowser(browser)
                                     ->toolbar()
                                     ->app_menu_button();
  DCHECK(app_menu_button);

  auto* bubble_view =
      new RelaunchRecommendedBubbleView(app_menu_button, detection_time);
  bubble_view->set_arrow(views::BubbleBorder::TOP_RIGHT);

  views::Widget* bubble_widget =
      views::BubbleDialogDelegateView::CreateBubble(bubble_view);
  bubble_view->ShowForReason(AUTOMATIC);

  return bubble_widget;
}

RelaunchRecommendedBubbleView::~RelaunchRecommendedBubbleView() = default;

bool RelaunchRecommendedBubbleView::Accept() {
  return true;
}

bool RelaunchRecommendedBubbleView::Close() {
  return true;
}

int RelaunchRecommendedBubbleView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK;
}

base::string16 RelaunchRecommendedBubbleView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  DCHECK_EQ(button, ui::DIALOG_BUTTON_OK);
  return l10n_util::GetStringUTF16(IDS_RELAUNCH_RECOMMENDED_ACCEPT_BUTTON);
}

base::string16 RelaunchRecommendedBubbleView::GetWindowTitle() const {
  const base::TimeDelta elapsed = base::TimeTicks::Now() - detection_time_;
  int message_id = IDS_RELAUNCH_RECOMMENDED_TITLE_MINUTES;
  int amount = elapsed.InMinutes();
  if (elapsed.InDays() > 0) {
    message_id = IDS_RELAUNCH_RECOMMENDED_TITLE_DAYS;
    amount = elapsed.InDays();
  } else if (elapsed.InHours() > 0) {
    message_id = IDS_RELAUNCH_RECOMMENDED_TITLE_HOURS;
    amount = elapsed.InHours();
  }

  return l10n_util::GetPluralStringFUTF16(message_id, amount);
}

bool RelaunchRecommendedBubbleView::ShouldShowCloseButton() const {
  return true;
}

gfx::ImageSkia RelaunchRecommendedBubbleView::GetWindowIcon() {
  // Harmony spec says 20pt icons.
  return gfx::CreateVectorIcon(gfx::IconDescription(
      vector_icons::kBusinessIcon, 20, gfx::kChromeIconGrey, base::TimeDelta(),
      gfx::kNoneIcon));
}

bool RelaunchRecommendedBubbleView::ShouldShowWindowIcon() const {
  return true;
}

void RelaunchRecommendedBubbleView::Init() {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  auto label = std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_RELAUNCH_RECOMMENDED_BODY),
      views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT);
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(label.release());

  // Start the timer for the next time the title neeeds to be updated (e.g.,
  // from "2 days" to "3 days").
  ScheduleNextTitleRefresh();
}

gfx::Size RelaunchRecommendedBubbleView::CalculatePreferredSize() const {
  const int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                        DISTANCE_BUBBLE_PREFERRED_WIDTH) -
                    margins().width();
  return gfx::Size(width, GetHeightForWidth(width));
}

RelaunchRecommendedBubbleView::RelaunchRecommendedBubbleView(
    views::View* anchor_view,
    base::TimeTicks detection_time)
    : LocationBarBubbleDelegateView(anchor_view, gfx::Point(), nullptr),
      detection_time_(detection_time) {
  chrome::RecordDialogCreation(chrome::DialogIdentifier::RELAUNCH_RECOMMENDED);
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::CONTROL, views::CONTROL));
}

void RelaunchRecommendedBubbleView::ScheduleNextTitleRefresh() {
  // Refresh at the next minute, hour, or day boundary; depending on the elapsed
  // time since detection.
  const base::TimeDelta elapsed = base::TimeTicks::Now() - detection_time_;
  base::TimeDelta delta = base::TimeDelta::FromMinutes(elapsed.InMinutes() + 1);
  if (elapsed.InDays() > 0)
    delta = base::TimeDelta::FromDays(elapsed.InDays() + 1);
  else if (elapsed.InHours() > 0)
    delta = base::TimeDelta::FromHours(elapsed.InHours() + 1);
  delta -= elapsed;

  refresh_timer_.Start(FROM_HERE, delta, this,
                       &RelaunchRecommendedBubbleView::OnTitleRefresh);
}

void RelaunchRecommendedBubbleView::OnTitleRefresh() {
  GetBubbleFrameView()->UpdateWindowTitle();
  ScheduleNextTitleRefresh();
}
