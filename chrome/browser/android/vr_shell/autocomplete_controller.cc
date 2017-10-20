// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/autocomplete_controller.h"

#include <string>
#include <utility>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/vr/autocomplete_match.h"
#include "chrome/browser/vr/browser_ui_interface.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_input.h"

namespace vr {

AutocompleteController::AutocompleteController(BrowserUiInterface* ui)
    : ui_(ui),
      profile_(ProfileManager::GetLastUsedProfile()),
      autocomplete_controller_(base::MakeUnique<::AutocompleteController>(
          base::MakeUnique<ChromeAutocompleteProviderClient>(profile_),
          this,
          AutocompleteClassifier::DefaultOmniboxProviders())) {}

AutocompleteController::~AutocompleteController() = default;

void AutocompleteController::HandleInput(const base::string16& text) {
  metrics::OmniboxEventProto::PageClassification page_classification =
      metrics::OmniboxEventProto::OTHER;
  autocomplete_controller_->Start(AutocompleteInput(
      text, page_classification, ChromeAutocompleteSchemeClassifier(profile_)));
}

void AutocompleteController::OnResultChanged(bool default_match_changed) {
  std::unique_ptr<VrAutocompleteResult> result;
  // LOG(INFO) << "Suggestions:";
  for (const auto& match : autocomplete_controller_->result()) {
    // LOG(INFO) << "  Suggestion:";
    // LOG(INFO) << "    " << match.contents;
    // LOG(INFO) << "    " << match.description;
    // LOG(INFO) << "    " << match.destination_url.spec();
    result->matches.emplace_back(vr::Suggestion());
    auto& item = result.matches.back();
    item.content = match.content;
    item.description = match.description;
    item.type = match.type;
  }

  ui_->SetAutocompleteResult(std::move(result));
}

}  // namespace vr
