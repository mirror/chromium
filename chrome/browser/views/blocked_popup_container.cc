// Copyright (c) 2008 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#include "chrome/browser/views/blocked_popup_container.h"

#include "chrome/app/theme/theme_resources.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/tab_contents.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/views/button.h"
#include "chrome/views/label.h"
#include "chrome/views/view.h"
#include "chrome/views/menu.h"
#include "chrome/views/text_button.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/gfx/path.h"

#include "generated_resources.h"

const int kNotifyMenuItem = -1;

// A number larger than the internal popup count on the Renderer; meant for
// preventing a compromised renderer from exhausting GDI memory by spawning
// infinite windows.
const int kImpossibleNumberOfPopups = 30;

const int kCloseButtonPadding = 5;

static const SkColor kBackgroundColor = SkColorSetRGB(222, 234, 248);
// TODO(erg): Get a real standard color for kBorderColor.
static const SkColor kBorderColor = SkColorSetRGB(111, 117, 124);
static const int kBorderSize = 1;
static const int kBackgroundCornerRadius = 4;

static const int kShowAnimationDurationMS = 200;
static const int kHideAnimationDurationMS = 120;
static const int kFramerate = 25;

// Rounded corner definition for the
static const SkScalar kRoundedCornerRad[8] = {
  // Top left corner
  SkIntToScalar(kBackgroundCornerRadius),
  SkIntToScalar(kBackgroundCornerRadius),
  // Top right corner
  SkIntToScalar(kBackgroundCornerRadius),
  SkIntToScalar(kBackgroundCornerRadius),
  // Bottom right corner
  0,
  0,
  // Bottom left corner
  0,
  0
};

////////////////////////////////////////////////////////////////////////////////
// BlockedPopupContainerView

class BlockedPopupContainerView : public views::View,
                                  public views::BaseButton::ButtonListener,
                                  public Menu::Delegate {
 public:
  BlockedPopupContainerView(BlockedPopupContainer* container);
  ~BlockedPopupContainerView();

  // 
  void UpdatePopupCountLabel(); // Do I need to do anything?

  // Overridden from views::View:
  virtual void Paint(ChromeCanvas* canvas);
  virtual void Layout();
  virtual gfx::Size GetPreferredSize();
  virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child) {
    View::ViewHierarchyChanged(is_add, parent, child);
  }

  // Overridden from views::ButtonListener::ButtonPressed:
  virtual void ButtonPressed(views::BaseButton* sender);

  // Overridden from Menu::Delegate:
  virtual bool IsItemChecked(int id) const;
  virtual void ExecuteCommand(int id);

 private:
  BlockedPopupContainer* container_;

  views::TextButton* popup_count_;
  views::Button* close_button_;

  /// Popup menu shown to user
  scoped_ptr<Menu> launch_menu_;
};

BlockedPopupContainerView::BlockedPopupContainerView(
    BlockedPopupContainer* container)
    : container_(container) {
  ResourceBundle &rb = ResourceBundle::GetSharedInstance();

  // Create a button with a multidigit number to reserve space.
  popup_count_ = new views::TextButton(
      l10n_util::GetStringF(IDS_POPUPS_BLOCKED_COUNT,
                            IntToWString(99)));
  popup_count_->SetListener(this, 1);
  AddChildView(popup_count_);

  // For now, we steal the Find close button, since it looks OK.
  close_button_ = new views::Button();
  close_button_->SetFocusable(true);
  close_button_->SetImage(views::Button::BS_NORMAL,
      rb.GetBitmapNamed(IDR_CLOSE_BAR));
  close_button_->SetImage(views::Button::BS_HOT,
      rb.GetBitmapNamed(IDR_CLOSE_BAR_H));
  close_button_->SetImage(views::Button::BS_PUSHED,
      rb.GetBitmapNamed(IDR_CLOSE_BAR_P));
  close_button_->SetListener(this, 0);
  AddChildView(close_button_);

  UpdatePopupCountLabel();
}

BlockedPopupContainerView::~BlockedPopupContainerView() {
}

void BlockedPopupContainerView::UpdatePopupCountLabel() {
  popup_count_->SetText(container_->GetWindowTitle());
  Layout();
  SchedulePaint();
}

void BlockedPopupContainerView::Paint(ChromeCanvas* canvas) {
  SkPaint paint;
  paint.setStyle(SkPaint::kFill_Style);
  paint.setFlags(SkPaint::kAntiAlias_Flag);
  paint.setColor(kBackgroundColor);

  SkRect rect;
  rect.set(0, 0, SkIntToScalar(width()), SkIntToScalar(height()));

  // Draw the border
  SkPaint border_paint;
  border_paint.setFlags(SkPaint::kAntiAlias_Flag);
  border_paint.setColor(kBorderColor);
  SkPath border_path;
  border_path.addRoundRect(rect, kRoundedCornerRad, SkPath::kCW_Direction);
  canvas->drawPath(border_path, border_paint);

  // Draw the bubble
  SkPath path;
  rect.set(SkIntToScalar(kBorderSize),
           SkIntToScalar(kBorderSize),
           SkIntToScalar(width() - kBorderSize),
           SkIntToScalar(height() - kBorderSize));
  path.addRoundRect(rect, kRoundedCornerRad, SkPath::kCW_Direction);
  canvas->drawPath(path, paint);
}

void BlockedPopupContainerView::Layout() {
  gfx::Size panel_size = GetPreferredSize();
  gfx::Size button_size = close_button_->GetPreferredSize();
  gfx::Size sz = popup_count_->GetPreferredSize();
  popup_count_->SetBounds(0, 0,
                          width() - button_size.width() - kCloseButtonPadding,
                          height());

  close_button_->SetBounds(width() - button_size.width() - kCloseButtonPadding,
                           kCloseButtonPadding,
                           button_size.width(), button_size.height());

  // TODO(erg): Unfinished?
}

gfx::Size BlockedPopupContainerView::GetPreferredSize() {
  gfx::Size prefsize = close_button_->GetPreferredSize();
  prefsize.Enlarge(popup_count_->GetPreferredSize().width(), 0);

  // Padding on all sides:
  prefsize.Enlarge(2 * kCloseButtonPadding, 2 * kCloseButtonPadding);

  return prefsize;
}

void BlockedPopupContainerView::ButtonPressed(views::BaseButton* sender) {
  if (sender == popup_count_) {
    // Menu goes here.
    launch_menu_.reset(new Menu(this, Menu::TOPLEFT, container_->GetHWND()));

    int item_count = container_->GetTabContentsCount();
    for (int i = 0; i < item_count; ++i) {
      std::wstring label = container_->GetDisplayStringForItem(i);
      // We can't just use the index into container_ here because Menu reserves
      // the value 0 as the nop command.
      launch_menu_->AppendMenuItem(i + 1, label, Menu::NORMAL);
    }

    launch_menu_->AppendSeparator();
    launch_menu_->AppendMenuItem(
        kNotifyMenuItem,
        l10n_util::GetString(IDS_OPTIONS_SHOWPOPUPBLOCKEDNOTIFICATION),
        Menu::NORMAL);

    CPoint cursor_pos;
    ::GetCursorPos(&cursor_pos);
    launch_menu_->RunMenuAt(cursor_pos.x, cursor_pos.y);
  } else if (sender == close_button_) {
    container_->CloseAllPopups();
    container_->set_dismissed();
  }
}

bool BlockedPopupContainerView::IsItemChecked(int id) const {
  if (id == kNotifyMenuItem)
    return container_->GetShowBlockedPopupNotification();

  return false;
}

void BlockedPopupContainerView::ExecuteCommand(int id) {
  if (id == kNotifyMenuItem) {
    container_->ToggleBlockedPopupNotification();
  } else {
    // Decrement id since all index based commands have 1 added to them. (See
    // ButtonPressed() for detail).
    container_->LaunchPopupIndex(id - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////
// BlockedPopupContainer

// static
BlockedPopupContainer* BlockedPopupContainer::Create(
    TabContents* owner, Profile* profile, const gfx::Point& initial_anchor) {
  BlockedPopupContainer* c = new BlockedPopupContainer(owner, profile);
  c->Init(initial_anchor);
  return c;
}

BlockedPopupContainer::~BlockedPopupContainer() {
}

BlockedPopupContainer::BlockedPopupContainer(TabContents* owner,
                                             Profile* profile)
    : Animation(kFramerate, NULL),
      owner_(owner), container_view_(NULL), has_been_dismissed_(false),
      in_show_animation_(false), visibility_percentage_(0) {
  block_popup_pref_.Init(
      prefs::kBlockPopups, profile->GetPrefs(), NULL);
}

void BlockedPopupContainer::ToggleBlockedPopupNotification() {
  bool current = block_popup_pref_.GetValue();
  block_popup_pref_.SetValue(!current);
}

bool BlockedPopupContainer::GetShowBlockedPopupNotification() {
  return ! block_popup_pref_.GetValue();
}

void BlockedPopupContainer::AddTabContents(TabContents* blocked_contents,
                                           const gfx::Rect& bounds) {
  if (has_been_dismissed_) {
    // We simply bounce this popup without notice.
    blocked_contents->CloseContents();
    return;
  }

  if (blocked_popups_.size() > kImpossibleNumberOfPopups) {
    blocked_contents->CloseContents();
    LOG(INFO) << "Warning: Renderer is sending more popups to us then should be"
        " possible. Renderer compromised?";
    return;
  }

  blocked_contents->set_delegate(this);
  blocked_popups_.push_back(std::make_pair(blocked_contents, bounds));
  container_view_->UpdatePopupCountLabel();

  ShowSelf();
}

void BlockedPopupContainer::LaunchPopupIndex(int index) {
  if (static_cast<size_t>(index) < blocked_popups_.size()) {
    TabContents* contents = blocked_popups_[index].first;
    gfx::Rect bounds = blocked_popups_[index].second;
    blocked_popups_.erase(blocked_popups_.begin() + index);
    container_view_->UpdatePopupCountLabel();

    contents->set_delegate(NULL);
    contents->DisassociateFromPopupCount();

    // Pass this TabContents back to our owner, forcing the window to be
    // displayed since user_gesture is true.
    owner_->AddNewContents(contents, NEW_POPUP, bounds, true);
  }

  if (blocked_popups_.size() == 0) {
    CloseAllPopups();
    // Note: we don't set the |has_been_dismissed_| flag here
  }
}

int BlockedPopupContainer::GetTabContentsCount() const {
  return blocked_popups_.size();
}

std::wstring BlockedPopupContainer::GetDisplayStringForItem(int index) {
  const GURL& url = blocked_popups_[index].first->GetURL().GetOrigin();

  std::wstring label;
  label += UTF8ToWide(url.possibly_invalid_spec());
  label += L" - ";
  label += blocked_popups_[index].first->GetTitle();
  return label;
}

void BlockedPopupContainer::CloseAllPopups() {
  CloseEachTabContents();

  container_view_->UpdatePopupCountLabel();

  HideSelf();
}

/////////////////////////////////////////////////////////////////////////////////
// Override from ConstrainedWindow:

void BlockedPopupContainer::CloseConstrainedWindow() {
  CloseEachTabContents();

  // Broadcast to all observers of NOTIFY_CWINDOW_CLOSED.
  // One example of such an observer is AutomationCWindowTracker in the
  // automation component.
  NotificationService::current()->Notify(NOTIFY_CWINDOW_CLOSED,
                                         Source<ConstrainedWindow>(this),
                                         NotificationService::NoDetails());

  Close();
}

void BlockedPopupContainer::RepositionConstrainedWindowTo(
    const gfx::Point& anchor_point) {
  anchor_point_ = anchor_point;
  SetPosition();
}

void BlockedPopupContainer::WasHidden() {
}

void BlockedPopupContainer::DidBecomeSelected() {
}

std::wstring BlockedPopupContainer::GetWindowTitle() const {
  return l10n_util::GetStringF(IDS_POPUPS_BLOCKED_COUNT,
                               IntToWString(GetTabContentsCount()));
}

const gfx::Rect& BlockedPopupContainer::GetCurrentBounds() const {
  return bounds_;
}

/////////////////////////////////////////////////////////////////////////////////
// Override from TabContentsDelegate:
void BlockedPopupContainer::OpenURLFromTab(TabContents* source,
                                           const GURL& url, const GURL& referrer,
                                           WindowOpenDisposition disposition,
                                           PageTransition::Type transition) {
  owner_->OpenURL(url, referrer, disposition, transition);
}

void BlockedPopupContainer::ReplaceContents(TabContents* source,
                                            TabContents* new_contents) {
  // Walk the vector to find the correct TabContents and replace it.
  bool found = false;
  gfx::Rect rect;
  for (std::vector<std::pair<TabContents*, gfx::Rect> >::iterator it =
           blocked_popups_.begin(); it != blocked_popups_.end(); ++it) {
    if (it->first == source) {
      rect = it->second;
      found = true;
      blocked_popups_.erase(it);
      break;
    }
  }

  if (found) {
    blocked_popups_.push_back(std::make_pair(new_contents, rect));
  }
}

void BlockedPopupContainer::AddNewContents(TabContents* source,
                                           TabContents* new_contents,
                                           WindowOpenDisposition disposition,
                                           const gfx::Rect& initial_pos,
                                           bool user_gesture) {
  owner_->AddNewContents(new_contents, disposition, initial_pos,
                         user_gesture);
}

void BlockedPopupContainer::CloseContents(TabContents* source) {
  for (std::vector<std::pair<TabContents*, gfx::Rect> >::iterator it =
           blocked_popups_.begin(); it != blocked_popups_.end(); ++it) {
    if (it->first == source) {
      blocked_popups_.erase(it);
      break;
    }
  }

  if (blocked_popups_.size() == 0)
    CloseAllPopups();
}

void BlockedPopupContainer::MoveContents(TabContents* source,
                                         const gfx::Rect& pos) {
  for (std::vector<std::pair<TabContents*, gfx::Rect> >::iterator it =
           blocked_popups_.begin(); it != blocked_popups_.end(); ++it) {
    if (it->first == source) {
      it->second = pos;
      break;
    }
  }
}

bool BlockedPopupContainer::IsPopup(TabContents* source) {
  return true;
}

TabContents* BlockedPopupContainer::GetConstrainingContents(
    TabContents* source) {
  return owner_;
}

/////////////////////////////////////////////////////////////////////////////////
// Override from Animation:
void BlockedPopupContainer::AnimateToState(double state) {
  if (in_show_animation_)
    visibility_percentage_ = state;
  else
    visibility_percentage_ = 1 - state;

  SetPosition();
}

/////////////////////////////////////////////////////////////////////////////////
// Override from views::ContainerWin:
void BlockedPopupContainer::OnFinalMessage(HWND window) {
  owner_->WillClose(this);
  CloseEachTabContents();
  ContainerWin::OnFinalMessage(window);
}

void BlockedPopupContainer::OnSize(UINT param, const CSize& size) {
  // Make a mask for the rounded corners at the top.
  SkRect rect;
  rect.set(0, 0, SkIntToScalar(size.cx), SkIntToScalar(size.cy));
  gfx::Path path;
  path.addRoundRect(rect, kRoundedCornerRad, SkPath::kCW_Direction);
  SetWindowRgn(path.CreateHRGN(), TRUE);

  ChangeSize(param, size);
}

// private:

void BlockedPopupContainer::Init(const gfx::Point& initial_anchor) {
  container_view_ = new BlockedPopupContainerView(this);
  container_view_->SetVisible(true);

  set_window_style(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
  ContainerWin::Init(owner_->GetContainerHWND(), gfx::Rect(), false);
  SetContentsView(container_view_);
  RepositionConstrainedWindowTo(initial_anchor);

  if (GetShowBlockedPopupNotification()) {
    ShowSelf();
  } else {
    has_been_dismissed_ = true;
  }
}

void BlockedPopupContainer::HideSelf() {
  in_show_animation_ = false;
  Animation::SetDuration(kHideAnimationDurationMS);
  Animation::Start();
}

void BlockedPopupContainer::ShowSelf() {
  SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
  if (!Animation::IsAnimating() && visibility_percentage_ < 1.0) {
    in_show_animation_ = true;
    Animation::SetDuration(kShowAnimationDurationMS);
    Animation::Start();
  }
}

void BlockedPopupContainer::SetPosition() {
  gfx::Size size = container_view_->GetPreferredSize();
  int base_x = anchor_point_.x() - size.width();
  int base_y = anchor_point_.y() - size.height();
  // The bounds we report through the automation system are the real bounds;
  // the animation is short lived...
  bounds_ = gfx::Rect(gfx::Point(base_x, base_y), size);

  int real_height = static_cast<int>(size.height() * visibility_percentage_);
  int real_y = anchor_point_.y() - real_height;

  // Size this window to the bottom left corner starting at the anchor point.
  if (real_height > 0) {
    SetWindowPos(HWND_TOP, base_x, real_y, size.width(), real_height, 0);
  } else {
    SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
  }
}

void BlockedPopupContainer::CloseEachTabContents() {
  for (std::vector<std::pair<TabContents*, gfx::Rect> >::iterator it =
          blocked_popups_.begin(); it != blocked_popups_.end(); ++it)
    it->first->CloseContents();

  blocked_popups_.clear();
}
