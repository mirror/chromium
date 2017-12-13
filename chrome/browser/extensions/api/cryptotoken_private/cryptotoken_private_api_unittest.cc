// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/cryptotoken_private/cryptotoken_private_api.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "chrome/browser/extensions/extension_api_unittest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/common/pref_names.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

using namespace api::cryptotoken_private;

class CryptoTokenPrivateApiTest : public extensions::ExtensionApiUnittest {
 public:
  CryptoTokenPrivateApiTest() {}
  ~CryptoTokenPrivateApiTest() override {}

 protected:
  bool GetSingleBooleanResult(
      UIThreadExtensionFunction* function, bool* result) {
    const base::ListValue* result_list = function->GetResultList();
    if (!result_list) {
      LOG(ERROR) << "Function has no result list.";
      return false;
    }

    if (result_list->GetSize() != 1u) {
      LOG(ERROR) << "Invalid number of results.";
      return false;
    }

    if (!result_list->GetBoolean(0, result)) {
      LOG(ERROR) << "Result is not boolean.";
      return false;
    }
    return true;
  }

  bool GetCanOriginAssertAppIdResult(const std::string& origin,
                                     const std::string& appId) {
    scoped_refptr<api::CryptotokenPrivateCanOriginAssertAppIdFunction> function(
        new api::CryptotokenPrivateCanOriginAssertAppIdFunction());
    function->set_has_callback(true);

    std::unique_ptr<base::ListValue> args(new base::ListValue);
    args->AppendString(origin);
    args->AppendString(appId);

    extension_function_test_utils::RunFunction(
        function.get(), std::move(args), browser(),
        extension_function_test_utils::NONE);

    bool result;
    CHECK(GetSingleBooleanResult(function.get(), &result));
    return result;
  }

  bool IsAppIdHashInEnterpriseContext(const std::string& appId) {
    scoped_refptr<api::CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction>
        function(
            new api::
                CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction());
    function->set_has_callback(true);

    std::unique_ptr<base::ListValue> args(new base::ListValue);
    args->AppendString(appId);

    extension_function_test_utils::RunFunction(
        function.get(), std::move(args), browser(),
        extension_function_test_utils::NONE);

    bool result;
    CHECK(GetSingleBooleanResult(function.get(), &result));
    return result;
  }
};

TEST_F(CryptoTokenPrivateApiTest, CanOriginAssertAppId) {
  std::string origin1("https://www.example.com");

  EXPECT_TRUE(GetCanOriginAssertAppIdResult(origin1, origin1));

  std::string same_origin_appid("https://www.example.com/appId");
  EXPECT_TRUE(GetCanOriginAssertAppIdResult(origin1, same_origin_appid));
  std::string same_etld_plus_one_appid("https://appid.example.com/appId");
  EXPECT_TRUE(GetCanOriginAssertAppIdResult(origin1, same_etld_plus_one_appid));
  std::string different_etld_plus_one_appid("https://www.different.com/appId");
  EXPECT_FALSE(GetCanOriginAssertAppIdResult(origin1,
                                             different_etld_plus_one_appid));

  // For legacy purposes, google.com is allowed to use certain appIds hosted at
  // gstatic.com.
  // TODO(juanlang): remove once the legacy constraints are removed.
  std::string google_origin("https://accounts.google.com");
  std::string gstatic_appid("https://www.gstatic.com/securitykey/origins.json");
  EXPECT_TRUE(GetCanOriginAssertAppIdResult(google_origin, gstatic_appid));
  // Not all gstatic urls are allowed, just those specifically whitelisted.
  std::string gstatic_otherurl("https://www.gstatic.com/foobar");
  EXPECT_FALSE(GetCanOriginAssertAppIdResult(google_origin, gstatic_otherurl));
}

static std::string SHA256Base64(base::StringPiece in) {
  std::string ret;
  base::Base64Encode(crypto::SHA256HashString(in), &ret);

  // Trim trailing '=' from the end of the base64 data in order to mirror what
  // the Javascript cryptotoken code does.
  while (!ret.empty() && ret.back() == '=') {
    ret.pop_back();
  }

  return ret;
}

TEST_F(CryptoTokenPrivateApiTest, IsAppIdHashInEnterpriseContext) {
  const std::string example_com("https://example.com/");
  const std::string example_com_hash(SHA256Base64(example_com));
  const std::string rp_id_hash(SHA256Base64("example.com"));
  const std::string foo_com_hash(SHA256Base64("https://foo.com/"));

  EXPECT_FALSE(IsAppIdHashInEnterpriseContext(example_com_hash));
  EXPECT_FALSE(IsAppIdHashInEnterpriseContext(foo_com_hash));
  EXPECT_FALSE(IsAppIdHashInEnterpriseContext(rp_id_hash));

  base::Value::ListStorage permitted_list;
  permitted_list.emplace_back(example_com);
  profile()->GetPrefs()->Set(prefs::kSecurityKeyPermitAttestation,
                             base::Value(permitted_list));

  EXPECT_TRUE(IsAppIdHashInEnterpriseContext(example_com_hash));
  EXPECT_FALSE(IsAppIdHashInEnterpriseContext(foo_com_hash));
  EXPECT_FALSE(IsAppIdHashInEnterpriseContext(rp_id_hash));
}

}  // namespace

}  // namespace extensions
