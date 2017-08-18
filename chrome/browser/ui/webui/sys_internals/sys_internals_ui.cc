// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sys_internals/sys_internals_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/sys_internals/sys_internals_message_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

SysInternalsUI::SysInternalsUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  // Set up the chrome://sys-internals source.
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUISysInternalsHost);

  // Localized strings.
  // html_source->AddLocalizedString("helloWorldTitle", IDS_HELLO_WORLD_TITLE);
  // html_source->AddLocalizedString("welcomeMessage",
  // IDS_HELLO_WORLD_WELCOME_TEXT);

  // As a demonstration of passing a variable for JS to use we pass in the name
  // "Bob". html_source->AddString("userName", "Bob");
  // html_source->SetJsonPath("strings.js");

  // Add required resources.
  html_source->AddResourcePath("index.css", IDR_SYS_INTERNALS_CSS);
  html_source->AddResourcePath("index.js", IDR_SYS_INTERNALS_JS);
  html_source->SetDefaultResource(IDR_SYS_INTERNALS_HTML);

  web_ui->AddMessageHandler(base::MakeUnique<SysInternalsMessageHandler>());

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html_source);
}

SysInternalsUI::~SysInternalsUI() {}