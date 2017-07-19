// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/global_confirm_info_bar.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/stl_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/core/infobar.h"
#include "ui/gfx/image/image.h"

// InfoBarDelegateProxy -------------------------------------------------------

class GlobalConfirmInfoBar::DelegateProxy : public ConfirmInfoBarDelegate {
 public:
  explicit DelegateProxy(scoped_refptr<GlobalConfirmInfoBar> global_info_bar);
  ~DelegateProxy() override;

 private:
  friend class GlobalConfirmInfoBar;

  // ConfirmInfoBarDelegate overrides
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;
  base::string16 GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  void InfoBarDismissed() override;

  infobars::InfoBar* info_bar_;
  scoped_refptr<GlobalConfirmInfoBar> global_info_bar_;

  DISALLOW_COPY_AND_ASSIGN(DelegateProxy);
};

GlobalConfirmInfoBar::DelegateProxy::DelegateProxy(
    scoped_refptr<GlobalConfirmInfoBar> global_info_bar)
    : info_bar_(nullptr), global_info_bar_(std::move(global_info_bar)) {}

GlobalConfirmInfoBar::DelegateProxy::~DelegateProxy() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
GlobalConfirmInfoBar::DelegateProxy::GetIdentifier() const {
  return global_info_bar_->delegate_->GetIdentifier();
}

base::string16 GlobalConfirmInfoBar::DelegateProxy::GetMessageText() const {
  return global_info_bar_->delegate_->GetMessageText();
}

int GlobalConfirmInfoBar::DelegateProxy::GetButtons() const {
  return global_info_bar_->delegate_->GetButtons();
}

base::string16 GlobalConfirmInfoBar::DelegateProxy::GetButtonLabel(
    InfoBarButton button) const {
  return global_info_bar_->delegate_->GetButtonLabel(button);
}

bool GlobalConfirmInfoBar::DelegateProxy::Accept() {
  scoped_refptr<GlobalConfirmInfoBar> global_info_bar = global_info_bar_;
  // Remove the current InfoBar (the one whose Accept button is being clicked)
  // from the control of GlobalConfirmInfoBar. This InfoBar will be closed by
  // caller of this method, and we don't need GlobalConfirmInfoBar to close it.
  // Furthermore, letting GlobalConfirmInfoBar close the current InfoBar can
  // cause memory corruption when InfoBar animation is disabled.
  global_info_bar->OnInfoBarRemoved(info_bar_, false);

  global_info_bar->delegate_->Accept();
  global_info_bar->Close();

  return true;
}

bool GlobalConfirmInfoBar::DelegateProxy::Cancel() {
  scoped_refptr<GlobalConfirmInfoBar> global_info_bar = global_info_bar_;

  // See comments in GlobalConfirmInfoBar::DelegateProxy::Accept().
  global_info_bar->OnInfoBarRemoved(info_bar_, false);

  global_info_bar->delegate_->Cancel();
  global_info_bar->Close();

  return true;
}

base::string16 GlobalConfirmInfoBar::DelegateProxy::GetLinkText() const {
  return global_info_bar_->delegate_->GetLinkText();
}

GURL GlobalConfirmInfoBar::DelegateProxy::GetLinkURL() const {
  return global_info_bar_->delegate_->GetLinkURL();
}

bool GlobalConfirmInfoBar::DelegateProxy::LinkClicked(
    WindowOpenDisposition disposition) {
  return global_info_bar_->delegate_->LinkClicked(disposition);
}

bool GlobalConfirmInfoBar::DelegateProxy::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate == this;
}

void GlobalConfirmInfoBar::DelegateProxy::InfoBarDismissed() {
  scoped_refptr<GlobalConfirmInfoBar> global_info_bar = global_info_bar_;

  // See comments in GlobalConfirmInfoBar::DelegateProxy::Accept().
  global_info_bar->OnInfoBarRemoved(info_bar_, false);

  global_info_bar->delegate_->InfoBarDismissed();
  global_info_bar->Close();
}

// GlobalConfirmInfoBar -------------------------------------------------------

// static
base::WeakPtr<GlobalConfirmInfoBar> GlobalConfirmInfoBar::Show(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate) {
  // The GlobalConfirmInfoBar is owned by every DelegateProxy that are created.
  // The instance gets deleted when all the infobar are removed.
  // Note: GlobalConfirmInfoBar is not owned until at least one infobar gets
  // created for a WebContents
  auto* global_info_bar = new GlobalConfirmInfoBar(std::move(delegate));
  return global_info_bar->weak_factory_.GetWeakPtr();
}

void GlobalConfirmInfoBar::Close() {
  while (!proxies_.empty()) {
    auto it = proxies_.begin();
    it->first->RemoveObserver(this);
    it->first->RemoveInfoBar(it->second->info_bar_);
    proxies_.erase(it);
  }
}

GlobalConfirmInfoBar::GlobalConfirmInfoBar(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate)
    : delegate_(std::move(delegate)),
      browser_tab_strip_tracker_(this, nullptr, nullptr),
      weak_factory_(this) {
  browser_tab_strip_tracker_.Init();
}

GlobalConfirmInfoBar::~GlobalConfirmInfoBar() = default;

void GlobalConfirmInfoBar::TabInsertedAt(TabStripModel* tab_strip_model,
                                         content::WebContents* web_contents,
                                         int index,
                                         bool foreground) {
  MaybeAddInfoBar(web_contents);
}

void GlobalConfirmInfoBar::TabChangedAt(content::WebContents* web_contents,
                                        int index,
                                        TabChangeType change_type) {
  MaybeAddInfoBar(web_contents);
}

void GlobalConfirmInfoBar::OnInfoBarRemoved(infobars::InfoBar* info_bar,
                                            bool animate) {
  // Do not process alien infobars.
  for (auto it : proxies_) {
    if (it.second->info_bar_ == info_bar) {
      OnManagerShuttingDown(info_bar->owner());
      break;
    }
  }
}

void GlobalConfirmInfoBar::OnManagerShuttingDown(
    infobars::InfoBarManager* manager) {
  manager->RemoveObserver(this);
  proxies_.erase(manager);
}

void GlobalConfirmInfoBar::MaybeAddInfoBar(content::WebContents* web_contents) {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);
  // WebContents from the tab strip must have the infobar service.
  DCHECK(infobar_service);

  // Skip if the |web_contents| already contains an infobar.
  if (base::ContainsKey(proxies_, infobar_service))
    return;

  std::unique_ptr<GlobalConfirmInfoBar::DelegateProxy> proxy(
      new GlobalConfirmInfoBar::DelegateProxy(this));
  GlobalConfirmInfoBar::DelegateProxy* proxy_ptr = proxy.get();
  infobars::InfoBar* added_bar = infobar_service->AddInfoBar(
      infobar_service->CreateConfirmInfoBar(std::move(proxy)));

  // It's possible for AddInfoBar() to fail.
  if (added_bar) {
    proxy_ptr->info_bar_ = added_bar;
    proxies_[infobar_service] = proxy_ptr;
    infobar_service->AddObserver(this);
  }
}
