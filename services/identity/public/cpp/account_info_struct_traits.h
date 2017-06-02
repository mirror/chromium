// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_INFO_STRUCT_TRAITS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_INFO_STRUCT_TRAITS_H_

#include <string>

#include "components/signin/core/browser/account_info.h"
#include "services/identity/public/interfaces/account_info.mojom.h"

namespace mojo {

template <>
struct StructTraits<identity::mojom::AccountInfo::DataView, ::AccountInfo> {
  static std::string account_id(const ::AccountInfo& r) { return r.account_id; }

  static std::string gaia(const ::AccountInfo& r) { return r.gaia; }

  static std::string email(const ::AccountInfo& r) { return r.email; }

  static std::string full_name(const ::AccountInfo& r) { return r.full_name; }

  static std::string given_name(const ::AccountInfo& r) { return r.given_name; }

  static std::string hosted_domain(const ::AccountInfo& r) {
    return r.hosted_domain;
  }

  static std::string locale(const ::AccountInfo& r) { return r.locale; }

  static std::string picture_url(const ::AccountInfo& r) {
    return r.picture_url;
  }

  static bool is_child_account(const ::AccountInfo& r) {
    return r.is_child_account;
  }

  static bool Read(identity::mojom::AccountInfo::DataView data,
                   ::AccountInfo* out) {
    std::string account_id;
    std::string gaia;
    std::string email;
    std::string full_name;
    std::string given_name;
    std::string hosted_domain;
    std::string locale;
    std::string picture_url;
    if (!data.ReadAccountId(&account_id) || !data.ReadGaia(&gaia) ||
        !data.ReadEmail(&email) || !data.ReadFullName(&full_name) ||
        !data.ReadGivenName(&given_name) ||
        !data.ReadHostedDomain(&hosted_domain) || !data.ReadLocale(&locale) ||
        !data.ReadPictureUrl(&picture_url)) {
      return false;
    }

    out->account_id = account_id;
    out->gaia = gaia;
    out->email = email;
    out->full_name = full_name;
    out->given_name = given_name;
    out->hosted_domain = hosted_domain;
    out->locale = locale;
    out->picture_url = picture_url;
    out->is_child_account = data.is_child_account();

    return true;
  }

  // Note that an AccountInfo being null cannot be defined as
  // !AccountInfo::IsValid(), as IsValid() requires that *every* field is
  // populated, which is too stringent a requirement.
  static bool IsNull(const ::AccountInfo& input) {
    return input.account_id.empty() || input.gaia.empty() ||
           input.email.empty();
  }

  static void SetToNull(::AccountInfo* output) { *output = AccountInfo(); }
};

}  // namespace mojo

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_INFO_STRUCT_TRAITS_H_
