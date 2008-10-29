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

#include "generated_resources.h"

const int kNotifyMenuItem = -1;

// A number larger than the internal popup count on the Renderer; meant for
// preventing a compromised renderer from exhausting GDI memory by spawning
// infinite windows.
const int kImpossibleNumberOfPopups = 30;

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
                            IntToWString(9001)));
  popup_count_->SetListener(this, 1);
  AddChildView(popup_count_);

  // For now, we steal the Find close button, since 
  close_button_ = new views::Button();
  close_button_->SetFocusable(true);
  close_button_->SetImage(views::Button::BS_NORMAL,
      rb.GetBitmapNamed(IDR_CLOSE_BAR));
  close_button_->SetImage(views::Button::BS_HOT,
      rb.GetBitmapNamed(IDR_CLOSE_BAR_H));
  close_button_->SetImage(views::Button::BS_PUSHED,
      rb.GetBitmapNamed(IDR_CLOSE_BAR_P));
  //  close_button_->SetTooltipText(
  //      l10n_util::GetString(IDS_FIND_IN_PAGE_CLOSE_TOOLTIP));
  close_button_->SetListener(this, 0);
  AddChildView(close_button_);

  UpdatePopupCountLabel();
}

BlockedPopupContainerView::~BlockedPopupContainerView() {
}

void BlockedPopupContainerView::UpdatePopupCountLabel() {
  popup_count_->SetText(
      l10n_util::GetStringF(IDS_POPUPS_BLOCKED_COUNT,
                            IntToWString(container_->GetTabContentsCount())));

  Layout();
  SchedulePaint();
}

void BlockedPopupContainerView::Paint(ChromeCanvas* canvas) {
  canvas->FillRectInt(SK_ColorRED, 0, 0, width(), height());
}

void BlockedPopupContainerView::Layout() {
  gfx::Size panel_size = GetPreferredSize();
  gfx::Size button_size = close_button_->GetPreferredSize();
  gfx::Size sz = popup_count_->GetPreferredSize();
  popup_count_->SetBounds(0, 0, width() - button_size.width(),
                          height());

  close_button_->SetBounds(width() - button_size.width(), 0,
                           button_size.width(), height());

  // TODO(erg): Unfinished?
}

gfx::Size BlockedPopupContainerView::GetPreferredSize() {
  gfx::Size prefsize = close_button_->GetPreferredSize();
  prefsize.Enlarge(popup_count_->GetPreferredSize().width(), 0);
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
    : owner_(owner), container_view_(NULL) {
  disable_popup_blocked_notification_pref_.Init(
      prefs::kBlockPopups, profile->GetPrefs(), NULL);
}

void BlockedPopupContainer::Init(const gfx::Point& initial_anchor) {
  container_view_ = new BlockedPopupContainerView(this);
  container_view_->SetVisible(true);

  ContainerWin::Init(owner_->GetContainerHWND(), gfx::Rect(), false);
  SetContentsView(container_view_);
  RepositionConstrainedWindowTo(initial_anchor);
}

void BlockedPopupContainer::ToggleBlockedPopupNotification() {
  bool current = disable_popup_blocked_notification_pref_.GetValue();
  disable_popup_blocked_notification_pref_.SetValue(!current);
}

bool BlockedPopupContainer::GetShowBlockedPopupNotification() {
  return disable_popup_blocked_notification_pref_.GetValue();
}

void BlockedPopupContainer::AddTabContents(TabContents* blocked_contents,
                                           const gfx::Rect& bounds) {
  if (blocked_popups_.size() > kImpossibleNumberOfPopups) {
    blocked_contents->CloseContents();
    LOG(INFO) << "Warning: Renderer is sending more popups to us then should be"
        " possible. Renderer compromised?";
    return;
  }

  blocked_popups_.push_back(std::make_pair(blocked_contents, bounds));
  container_view_->UpdatePopupCountLabel();

  // It is possible we were hidden and now we don't want to be.
  SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void BlockedPopupContainer::LaunchPopupIndex(int index) {
  if (static_cast<size_t>(index) < blocked_popups_.size()) {
    TabContents* contents = blocked_popups_[index].first;
    gfx::Rect bounds = blocked_popups_[index].second;
    blocked_popups_.erase(blocked_popups_.begin() + index);
    container_view_->UpdatePopupCountLabel();

    contents->DisassociateFromPopupCount();

    // Pass this TabContents back to our owner, forcing the window to be
    // displayed since user_gesture is true.
    owner_->AddNewContents(contents, NEW_POPUP, bounds, true);

    // TODO: If the above works, try to rip out all the H4X code about
    // transitioning from a constrained to a normal window.
  }

  if (blocked_popups_.size() == 0) {
    CloseAllPopups();
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
  for(std::vector<std::pair<TabContents*, gfx::Rect> >::iterator it =
          blocked_popups_.begin(); it != blocked_popups_.end(); ++it) {
    it->first->CloseContents();
  }
  blocked_popups_.clear();

  container_view_->UpdatePopupCountLabel();

  // Size this window to the bottom left corner starting at the anchor point.
  SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
}

// ConstrainedWindow interface. Needs work.
void BlockedPopupContainer::ActivateConstrainedWindow() { }
void BlockedPopupContainer::CloseConstrainedWindow() {
  // Close everything we own.
  for(std::vector<std::pair<TabContents*, gfx::Rect> >::iterator it =
          blocked_popups_.begin(); it != blocked_popups_.end(); ++it) {
    it->first->CloseContents();
  }
  blocked_popups_.clear();

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
  gfx::Size size = container_view_->GetPreferredSize();
  int x = anchor_point.x() - size.width();
  int y = anchor_point.y() - size.height();

  // Size this window to the bottom left corner starting at the anchor point.
  SetWindowPos(HWND_TOP, x, y, size.width(), size.height(), SWP_SHOWWINDOW);
}

void BlockedPopupContainer::WasHidden() { }
void BlockedPopupContainer::DidBecomeSelected() { }
std::wstring BlockedPopupContainer::GetWindowTitle() const {
  return std::wstring();
}
void BlockedPopupContainer::UpdateWindowTitle() { }
const gfx::Rect& BlockedPopupContainer::GetCurrentBounds() const {
  return bounds_;
}
