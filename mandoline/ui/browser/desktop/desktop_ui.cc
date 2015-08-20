// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mandoline/ui/browser/desktop/desktop_ui.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "mandoline/ui/aura/native_widget_view_manager.h"
#include "mandoline/ui/browser/browser_manager.h"
#include "mandoline/ui/browser/public/interfaces/omnibox.mojom.h"
#include "mojo/common/common_type_converters.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/widget/widget_delegate.h"

namespace mandoline {

class ProgressView : public views::View {
 public:
  ProgressView() : progress_(0.f), loading_(false) {}
  ~ProgressView() override {}

  void SetProgress(double progress) {
    progress_ = progress;
    SchedulePaint();
  }

  void SetIsLoading(bool loading) {
    if (loading == loading_)
      return;

    loading_ = loading;
    progress_ = 0.f;
    SchedulePaint();
  }

 private:
  void OnPaint(gfx::Canvas* canvas) override {
    if (loading_) {
      canvas->FillRect(GetLocalBounds(), SK_ColorGREEN);

      gfx::Rect progress_rect = GetLocalBounds();
      progress_rect.set_width(progress_rect.width() * progress_);
      canvas->FillRect(progress_rect, SK_ColorRED);
    } else {
      canvas->FillRect(GetLocalBounds(), SK_ColorGRAY);
    }
  }

  double progress_;
  bool loading_;

  DISALLOW_COPY_AND_ASSIGN(ProgressView);
};

////////////////////////////////////////////////////////////////////////////////
// DesktopUI, public:

DesktopUI::DesktopUI(mojo::ApplicationImpl* application_impl,
                     BrowserManager* manager)
    : application_impl_(application_impl),
      browser_(application_impl, this),
      manager_(manager),
      omnibox_launcher_(nullptr),
      progress_bar_(nullptr),
      root_(nullptr),
      content_(nullptr),
      omnibox_(nullptr) {}

DesktopUI::~DesktopUI() {}

////////////////////////////////////////////////////////////////////////////////
// DesktopUI, BrowserUI implementation:

void DesktopUI::Init(mojo::View* root) {
  DCHECK_GT(root->viewport_metrics().device_pixel_ratio, 0);
  if (!aura_init_)
    aura_init_.reset(new AuraInit(root, application_impl_->shell()));

  root_ = root;
  omnibox_ = root_->view_manager()->CreateView();
  root_->AddChild(omnibox_);

  views::WidgetDelegateView* widget_delegate = new views::WidgetDelegateView;
  widget_delegate->GetContentsView()->set_background(
    views::Background::CreateSolidBackground(0xFFDDDDDD));
  omnibox_launcher_ =
      new views::LabelButton(this, base::ASCIIToUTF16("Open Omnibox"));
  progress_bar_ = new ProgressView;

  widget_delegate->GetContentsView()->AddChildView(omnibox_launcher_);
  widget_delegate->GetContentsView()->AddChildView(progress_bar_);
  widget_delegate->GetContentsView()->SetLayoutManager(this);

  views::Widget* widget = new views::Widget;
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.native_widget =
      new NativeWidgetViewManager(widget, application_impl_->shell(), root_);
  params.delegate = widget_delegate;
  params.bounds = root_->bounds().To<gfx::Rect>();
  widget->Init(params);
  widget->Show();
  root_->SetFocus();
}

void DesktopUI::LoadURL(const GURL& url) {
  browser_.LoadURL(url);
}

void DesktopUI::ViewManagerDisconnected() {
  manager_->BrowserUIClosed(this);
}

void DesktopUI::EmbedOmnibox(mojo::ApplicationConnection* connection) {
  mojo::ViewManagerClientPtr view_manager_client;
  connection->ConnectToService(&view_manager_client);
  omnibox_->Embed(view_manager_client.Pass());

  // TODO(beng): This should be handled sufficiently by
  //             OmniboxImpl::ShowWindow() but unfortunately view manager policy
  //             currently prevents the embedded app from changing window z for
  //             its own window.
  omnibox_->MoveToFront();
}

void DesktopUI::OnURLChanged() {
  omnibox_launcher_->SetText(base::UTF8ToUTF16(browser_.current_url().spec()));
}

void DesktopUI::LoadingStateChanged(bool loading) {
  progress_bar_->SetIsLoading(loading);
}

void DesktopUI::ProgressChanged(double progress) {
  progress_bar_->SetProgress(progress);
}

////////////////////////////////////////////////////////////////////////////////
// DesktopUI, views::LayoutManager implementation:

gfx::Size DesktopUI::GetPreferredSize(const views::View* view) const {
  return gfx::Size();
}

void DesktopUI::Layout(views::View* host) {
  gfx::Rect omnibox_launcher_bounds = host->bounds();
  omnibox_launcher_bounds.Inset(10, 10, 10, host->bounds().height() - 40);
  omnibox_launcher_->SetBoundsRect(omnibox_launcher_bounds);

  gfx::Rect progress_bar_bounds(omnibox_launcher_bounds.x(),
                                omnibox_launcher_bounds.bottom() + 2,
                                omnibox_launcher_bounds.width(),
                                5);
  progress_bar_->SetBoundsRect(progress_bar_bounds);

  mojo::Rect content_bounds_mojo;
  content_bounds_mojo.x = progress_bar_bounds.x();
  content_bounds_mojo.y = progress_bar_bounds.bottom() + 10;
  content_bounds_mojo.width = progress_bar_bounds.width();
  content_bounds_mojo.height =
      host->bounds().height() - content_bounds_mojo.y - 10;
  browser_.content()->SetBounds(content_bounds_mojo);
  omnibox_->SetBounds(
      mojo::TypeConverter<mojo::Rect, gfx::Rect>::Convert(host->bounds()));
}

////////////////////////////////////////////////////////////////////////////////
// DesktopUI, views::ButtonListener implementation:

void DesktopUI::ButtonPressed(views::Button* sender, const ui::Event& event) {
  DCHECK_EQ(sender, omnibox_launcher_);
  browser_.ShowOmnibox();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserUI, public:

// static
BrowserUI* BrowserUI::Create(mojo::ApplicationImpl* application_impl,
                             BrowserManager* manager) {
  return new DesktopUI(application_impl, manager);
}

}  // namespace mandoline
