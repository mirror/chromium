// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/fake_account_fetcher_service.h"

#include "base/values.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"

FakeAccountFetcherService::FakeAccountFetcherService() {}

void FakeAccountFetcherService::FakeUserInfoFetchSuccess(
    const std::string& account_id,
    const std::string& email,
    const std::string& gaia,
    const std::string& hosted_domain,
    const std::string& full_name,
    const std::string& given_name,
    const std::string& locale,
    const std::string& picture_url) {
  base::DictionaryValue user_info;
  user_info.SetKey("id", base::Value(gaia));
  user_info.SetKey("email", base::Value(email));
  user_info.SetKey("hd", base::Value(hosted_domain));
  user_info.SetKey("name", base::Value(full_name));
  user_info.SetKey("given_name", base::Value(given_name));
  user_info.SetKey("locale", base::Value(locale));
  user_info.SetKey("picture", base::Value(picture_url));
  account_tracker_service()->SetAccountStateFromUserInfo(account_id,
                                                         &user_info);
}

void FakeAccountFetcherService::FakeSetIsChildAccount(
    const std::string& account_id,
    const bool& is_child_account) {
  SetIsChildAccount(account_id, is_child_account);
}

void FakeAccountFetcherService::StartFetchingUserInfo(
    const std::string& account_id) {
  // In tests, don't do actual network fetch.
}

void FakeAccountFetcherService::StartFetchingChildInfo(
    const std::string& account_id) {
  // In tests, don't do actual network fetch.
}
