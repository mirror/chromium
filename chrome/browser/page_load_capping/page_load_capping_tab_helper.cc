// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_capping/page_load_capping_tab_helper.h"

#include "base/bind.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/page_load_capping/page_load_capping_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {
void CreateInfoBar(content::WebContents* web_contents,
                   int64_t kilo_bytes,
                   bool is_paused,
                   infobars::InfoBar* info_bar,
                   bool is_video) {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);

  std::unique_ptr<PageLoadCappingInfoBarDelegate> delegate(
      new PageLoadCappingInfoBarDelegate(web_contents, kilo_bytes, is_paused,
                                         is_video));

  std::unique_ptr<infobars::InfoBar> new_infobar(
      infobar_service->CreateConfirmInfoBar(std::move(delegate)));
  if (info_bar) {
    infobar_service->ReplaceInfoBar(info_bar, std::move(new_infobar));
  } else {
    infobar_service->AddInfoBar(std::move(new_infobar));
  }
}
}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PageLoadCappingTabHelper);

class PageLoadCappingTabHelper::PageLoadCappingInfoBarObserver
    : public infobars::InfoBarManager::Observer {
 public:
  PageLoadCappingInfoBarObserver() : infobar_(nullptr) {}
  ~PageLoadCappingInfoBarObserver() override {}

  void OnInfoBarAdded(infobars::InfoBar* infobar) override {
    if (infobar->delegate()->AsPageLoadCappingInfoBarDelegate()) {
      infobar_ = infobar;
    }
  }

  void OnInfoBarRemoved(infobars::InfoBar* infobar, bool animate) override {
    if (infobar_ == infobar) {
      infobar_ = nullptr;
    }
  }

  void OnInfoBarReplaced(infobars::InfoBar* old_infobar,
                         infobars::InfoBar* new_infobar) override {
    if (old_infobar == infobar_)
      infobar_ = new_infobar;
  }
  void OnManagerShuttingDown(infobars::InfoBarManager* manager) override {
    manager->RemoveObserver(this);
  }

  infobars::InfoBar* GetInfobar() { return infobar_; }

 private:
  infobars::InfoBar* infobar_;

  DISALLOW_COPY_AND_ASSIGN(PageLoadCappingInfoBarObserver);
};

PageLoadCappingTabHelper::~PageLoadCappingTabHelper() {}

PageLoadCappingTabHelper::PageLoadCappingTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      infobar_observer_(
          new PageLoadCappingTabHelper::PageLoadCappingInfoBarObserver()) {
  InfoBarService::FromWebContents(web_contents)
      ->AddObserver(infobar_observer_.get());
}

void PageLoadCappingTabHelper::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument())
    return;
  committed_ = false;
  navigation_id_ = navigation_handle->GetGlobalRequestID();
}

void PageLoadCappingTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() || navigation_handle->IsSameDocument())
    return;
  info_bar_created_ = false;
  committed_ = true;
  paused_ = false;
}

void PageLoadCappingTabHelper::UpdateInfobar(int64_t kilo_bytes,
                                             bool is_video) {
  if (!committed_)
    return;
  if (!info_bar_created_)
    is_video_ = is_video;
  if (is_video != is_video_)
    return;
  last_data_update_ = kilo_bytes;
  if (info_bar_created_)
    return;
  CreateInfoBar(web_contents(), last_data_update_, false, nullptr, is_video_);
  info_bar_created_ = true;
}

void PageLoadCappingTabHelper::Pause() {
  paused_ = true;
  auto* info_bar = infobar_observer_->GetInfobar();

  web_contents()->BlockRequests();

  CreateInfoBar(web_contents(), last_data_update_, true, info_bar, is_video_);
}

void PageLoadCappingTabHelper::UnPause(bool create_new) {
  paused_ = false;
  auto* info_bar = infobar_observer_->GetInfobar();

  web_contents()->ResumeBlockedRequests();
  if (create_new && !create_new) {
    CreateInfoBar(web_contents(), last_data_update_, false, info_bar,
                  is_video_);
  }
  if (!info_bar)
    return;
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents());

  infobar_service->RemoveInfoBar(info_bar);
}

void PageLoadCappingTabHelper::StopVideo() {
  LOG(WARNING) << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&7";
  content::MediaSession* media_session =
      content::MediaSession::Get(web_contents());
  LOG(WARNING) << media_session;
  if (media_session)
    media_session->Suspend(content::MediaSession::SuspendType::UI);

  auto* info_bar = infobar_observer_->GetInfobar();
  if (!info_bar)
    return;
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents());

  infobar_service->RemoveInfoBar(info_bar);
  info_bar_created_ = false;
}
