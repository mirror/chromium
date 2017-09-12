// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions/interventions_ui.h"

#include <string>
#include <unordered_set>

#include "base/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

content::WebUIDataSource* CreateInterventionsHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIInterventionsHost);

  source->SetDefaultResource(IDR_INTERVENTIONS_INDEX_HTML);
  source->UseGzip(std::unordered_set<std::string>());
  return source;
}

}  // namespace

InterventionsUI::InterventionsUI(content::WebUI* web_ui)
    : WebUIController(web_ui) {
  // Set up the chrome://interventions/ source.
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateInterventionsHTMLSource());

  // TODO(thanhdle): Add a previews message handler.
  // crbug.com/764409
}
