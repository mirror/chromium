// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin_partition_manager.h"

#include "base/callback.h"
#include "base/guid.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/chromeos/login/signin_certs_user_data.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "net/base/escape.h"
#include "url/gurl.h"

namespace chromeos {
namespace login {

namespace {
std::string GeneratePartitionName() {
  return base::GenerateGUID();
}
}  // namespace

SigninPartitionManager::SigninPartitionManager(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {}

SigninPartitionManager::~SigninPartitionManager() {}

void SigninPartitionManager::StartSigninSession(
    const content::WebContents* host_webcontents) {
  // If we already are in a sign-in session, close it first.
  // This clears stale data from the last-used StorageParittion.
  CloseCurrentSigninSession(base::Bind(&base::DoNothing));

  storage_partition_domain_ = host_webcontents->GetLastCommittedURL().host();
  current_storage_partition_name_ = GeneratePartitionName();

  std::string url_encoded_partition =
      net::EscapeQueryParamValue(current_storage_partition_name_, false);
  GURL guest_site(base::StringPrintf(
      "%s://%s/%s?%s", content::kGuestScheme, storage_partition_domain_.c_str(),
      "" /* not permanent */, url_encoded_partition.c_str()));
  current_storage_partition_ =
      content::BrowserContext::GetStoragePartitionForSite(browser_context_,
                                                          guest_site, true);
  SigninCertsUserData* signin_certs_user_data =
      SigninCertsUserData::CreateForStoragePartition(
          current_storage_partition_);
  signin_certs_user_data->set_allow_client_certificates(true);
}

void SigninPartitionManager::CloseCurrentSigninSession(
    base::Closure partition_data_cleared) {
  if (!GetCurrentStoragePartition()) {
    partition_data_cleared.Run();
  } else {
    ClearCurrentStoragePartition(partition_data_cleared);
    current_storage_partition_ = nullptr;
    current_storage_partition_name_.clear();
  }
}

void SigninPartitionManager::ClearCurrentStoragePartition(
    base::Closure partition_data_cleared) {
  DCHECK(current_storage_partition_);
  current_storage_partition_->ClearData(
      content::StoragePartition::REMOVE_DATA_MASK_ALL,
      content::StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL, GURL(),
      content::StoragePartition::OriginMatcherFunction(), base::Time(),
      base::Time::Now(), partition_data_cleared);
}

const std::string& SigninPartitionManager::GetCurrentStoragePartitionName() {
  return current_storage_partition_name_;
}

content::StoragePartition*
SigninPartitionManager::GetCurrentStoragePartition() {
  return current_storage_partition_;
}

}  // namespace login
}  // namespace chromeos
