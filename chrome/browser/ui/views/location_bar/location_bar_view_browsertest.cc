// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/command_updater_delegate.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/passwords/manage_passwords_ui_controller.h"
#include "chrome/browser/ui/search/search_tab_helper.h"
#include "chrome/browser/ui/tab_dialogs.h"
#include "chrome/browser/ui/toolbar/chrome_toolbar_model_delegate.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/toolbar/toolbar_model_impl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_constants.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

using content::WebContents;

namespace {

class NoopCommandUpdaterDelegate final : public CommandUpdaterDelegate {
 public:
  NoopCommandUpdaterDelegate() = default;
  ~NoopCommandUpdaterDelegate() final = default;

 private:
  void ExecuteCommandWithDisposition(int id,
                                     WindowOpenDisposition disposition) final {}

  DISALLOW_COPY_AND_ASSIGN(NoopCommandUpdaterDelegate);
};

class WebContentsOnlyDelegate final : public LocationBarView::Delegate,
                                      public ChromeToolbarModelDelegate {
 public:
  explicit WebContentsOnlyDelegate(std::unique_ptr<WebContents> web_contents)
      : web_contents_(std::move(web_contents)),
        toolbar_model_(
            std::make_unique<ToolbarModelImpl>(this,
                                               content::kMaxURLDisplayChars)) {
    SecurityStateTabHelper::CreateForWebContents(web_contents_.get());
    ChromeTranslateClient::CreateForWebContents(web_contents_.get());
    autofill::ChromeAutofillClient::CreateForWebContents(web_contents_.get());
    ChromePasswordManagerClient::CreateForWebContentsWithAutofillClient(
        web_contents_.get(),
        autofill::ChromeAutofillClient::FromWebContents(web_contents_.get()));
    ManagePasswordsUIController::CreateForWebContents(web_contents_.get());
    SearchTabHelper::CreateForWebContents(web_contents_.get());
    TabDialogs::CreateForWebContents(web_contents_.get());
  }
  ~WebContentsOnlyDelegate() final = default;

 private:
  // LocationBarView::Delegate overrides.
  WebContents* GetWebContents() final { return web_contents_.get(); }
  ToolbarModel* GetToolbarModel() final { return toolbar_model_.get(); }
  const ToolbarModel* GetToolbarModel() const final {
    return toolbar_model_.get();
  }
  ContentSettingBubbleModelDelegate* GetContentSettingBubbleModelDelegate()
      final {
    return nullptr;
  }

  // ChromeToolbarModelDelegate overrides.
  WebContents* GetActiveWebContents() const final {
    return web_contents_.get();
  }

  const std::unique_ptr<WebContents> web_contents_;
  const std::unique_ptr<ToolbarModelImpl> toolbar_model_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsOnlyDelegate);
};

class LocationBarViewOnlyWidgetDelegate final
    : public views::WidgetDelegateView {
 public:
  explicit LocationBarViewOnlyWidgetDelegate(Profile* profile)
      : profile_(profile), command_updater_(&noop_command_updater_delegate_) {}
  ~LocationBarViewOnlyWidgetDelegate() final = default;

  void Init() {
    SetLayoutManager(new views::FillLayout());

    delegate_ = std::make_unique<WebContentsOnlyDelegate>(base::WrapUnique(
        WebContents::Create(WebContents::CreateParams(profile_))));
    location_bar_view_ = new LocationBarView(
        nullptr, profile_, &command_updater_, delegate_.get(), true);

    AddChildView(location_bar_view_);

    location_bar_view_->Init();
  }

  LocationBarView* location_bar_view() { return location_bar_view_; }

 private:
  views::View* GetContentsView() final { return this; }

  Profile* const profile_;
  NoopCommandUpdaterDelegate noop_command_updater_delegate_;
  CommandUpdater command_updater_;
  std::unique_ptr<WebContentsOnlyDelegate> delegate_ = nullptr;
  LocationBarView* location_bar_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(LocationBarViewOnlyWidgetDelegate);
};

}  // namespace

class LocationBarViewBrowserTest : public InProcessBrowserTest {
 public:
  LocationBarViewBrowserTest() : widget_(new views::Widget()) {}

 protected:
  void SetUpOnMainThread() final {
    InProcessBrowserTest::SetUpOnMainThread();

    views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
    widget_delegate_ =
        new LocationBarViewOnlyWidgetDelegate(browser()->profile());
    params.delegate = widget_delegate_;
    widget_->Init(params);
    widget_delegate_->Init();
  }

  LocationBarView* location_bar_view() {
    return widget_delegate_->location_bar_view();
  }

  views::Widget* widget_;
  LocationBarViewOnlyWidgetDelegate* widget_delegate_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(LocationBarViewBrowserTest, LayoutWithoutBrowser) {
  location_bar_view()->GetPreferredSize();
  location_bar_view()->Layout();
}
