// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/input_window_dialog_ui.h"

#include "chrome/browser/ui/webui/chrome_web_ui_data_source.h"
#include "chrome/common/url_constants.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/profiles/profile.h"
#include "grit/browser_resources.h"
#include "grit/generated_resources.h"

InputWindowDialogUI::InputWindowDialogUI(TabContents* contents)
    : HtmlDialogUI(contents) {
  ChromeWebUIDataSource* source =
      new ChromeWebUIDataSource(chrome::kChromeUIInputWindowDialogHost);

  source->AddLocalizedString("ok", IDS_OK);
  source->AddLocalizedString("cancel", IDS_CANCEL);

  // Set the json path.
  source->set_json_path("strings.js");

  // Set default resource.
  source->set_default_resource(IDR_INPUT_WINDOW_DIALOG_HTML);

  Profile* profile = Profile::FromBrowserContext(contents->browser_context());
  profile->GetChromeURLDataManager()->AddDataSource(source);
}

InputWindowDialogUI::~InputWindowDialogUI() {
}
