// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/first_run_bubble_presenter.h"

#include "chrome/browser/search_engines/template_url_service_factory.h"

// static
void FirstRunBubblePresenter::PresentWhenReady(
    Profile* profile,
    const FirstRunBubblePresenter::PresentCallback& present_callback) {
  TemplateURLService* url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  if (url_service->loaded())
    return present_callback.Run();
  new FirstRunBubblePresenter(url_service, std::move(present_callback));
}

FirstRunBubblePresenter::FirstRunBubblePresenter(
    TemplateURLService* url_service,
    const PresentCallback& present_callback) {
  template_url_service_ = url_service;
  present_callback_ = std::move(present_callback);

  template_url_service_->AddObserver(this);
  template_url_service_->Load();
}

FirstRunBubblePresenter::~FirstRunBubblePresenter() {}

void FirstRunBubblePresenter::OnTemplateURLServiceChanged() {
  // Don't actually self-destruct or run the callback until the
  // TemplateURLService is really loaded.
  if (!template_url_service_->loaded())
    return;
  template_url_service_->RemoveObserver(this);
  template_url_service_ = nullptr;
  present_callback_.Run();
  delete this;
}
