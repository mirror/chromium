// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/mock_autocomplete_provider_client.h"

#include "base/memory/ptr_util.h"
#include "base/test/null_task_runner.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "net/url_request/url_request_test_util.h"

MockAutocompleteProviderClient::MockAutocompleteProviderClient()
    : signin_client_(&pref_service_),
      signin_manager_(&signin_client_, &account_tracker_) {
  token_service_ = base::MakeUnique<FakeOAuth2TokenService>();
  request_context_getter_ =
      new net::TestURLRequestContextGetter(base::ThreadTaskRunnerHandle::Get());
  contextual_suggestions_service_ =
      base::MakeUnique<ContextualSuggestionsService>(
          &signin_manager_, token_service_.get(), template_url_service_.get(),
          request_context_getter_.get());
}

MockAutocompleteProviderClient::~MockAutocompleteProviderClient() {
}
