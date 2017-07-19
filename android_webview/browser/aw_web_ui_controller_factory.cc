// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_web_ui_controller_factory.h"

#include "components/safe_browsing/web_ui/constants.h"
#include "components/safe_browsing/web_ui/safe_browsing_ui.h"
#include "content/public/browser/web_ui.h"

using content::WebUI;
using content::WebUIController;

namespace {

// A function for creating a new WebUI. The caller owns the return value, which
// may be nullptr (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

// Template for defining WebUIFactoryFunction.
template <class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}

WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui, const GURL& url) {
  if (url.host() == safe_browsing::kChromeUISafeBrowsingHost) {
    return &NewWebUI<safe_browsing::SafeBrowsingUI>;
  }

  return nullptr;
}

}  // namespace

namespace android_webview {

// static
AwWebUIControllerFactory* AwWebUIControllerFactory::GetInstance() {
  return base::Singleton<AwWebUIControllerFactory>::get();
}

AwWebUIControllerFactory::AwWebUIControllerFactory() {}

AwWebUIControllerFactory::~AwWebUIControllerFactory() {}

WebUI::TypeID AwWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  WebUIFactoryFunction function = GetWebUIFactoryFunction(nullptr, url);
  return function ? reinterpret_cast<WebUI::TypeID>(function) : WebUI::kNoWebUI;
}

bool AwWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return GetWebUIType(browser_context, url) != WebUI::kNoWebUI;
}

bool AwWebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return UseWebUIForURL(browser_context, url);
}

WebUIController* AwWebUIControllerFactory::CreateWebUIControllerForURL(
    WebUI* web_ui,
    const GURL& url) const {
  WebUIFactoryFunction function = GetWebUIFactoryFunction(web_ui, url);
  if (!function)
    return nullptr;

  return (*function)(web_ui, url);
}

}  // namespace android_webview
