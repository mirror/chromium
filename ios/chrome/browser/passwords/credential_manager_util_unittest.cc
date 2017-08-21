// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/credential_manager_util.h"

#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using password_manager::CredentialInfo;
using password_manager::CredentialMediationRequirement;
using password_manager::CredentialType;

namespace {

constexpr char kTestWebOrigin[] = "https://example.com/";
constexpr char kTestIconURL[] = "https://www.google.com/favicon.ico";

}  // namespace

class CredentialManagerUtilTest : public PlatformTest {
 public:
  CredentialManagerUtilTest() {}

  ~CredentialManagerUtilTest() override {}

 protected:
  // Builds a dictionary with example common fields of PasswordCredential and
  // FederatedCredential.
  base::DictionaryValue BuildExampleCredential() {
    base::DictionaryValue json;
    json.SetString("id", "john@doe.com");
    json.SetString("name", "John Doe");
    json.SetString("iconURL", kTestIconURL);
    return json;
  }

  // Builds a dictionary representing valid PasswordCredential.
  base::DictionaryValue BuildExampleValidPasswordCredential() {
    base::DictionaryValue json = BuildExampleCredential();
    json.SetString("password", "admin123");
    json.SetString("type", "PasswordCredential");
    return json;
  }

  // Builds a dictionary representing valid FederatedCredential.
  base::DictionaryValue BuildExampleValidFederatedCredential() {
    base::DictionaryValue json = BuildExampleCredential();
    json.SetString("provider", kTestWebOrigin);
    json.SetString("type", "FederatedCredential");
    return json;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CredentialManagerUtilTest);
};

// Checks that CredentialRequestOptions.password field is parsed
// correctly.
TEST_F(CredentialManagerUtilTest, ParseIncludePasswords) {
  base::DictionaryValue json;
  bool include_passwords = true;

  // Default value should be false.
  EXPECT_TRUE(ParseIncludePasswords(json, &include_passwords));
  EXPECT_FALSE(include_passwords);

  // true/false values should be parsed correctly.
  json.SetBoolean("password", true);
  EXPECT_TRUE(ParseIncludePasswords(json, &include_passwords));
  EXPECT_TRUE(include_passwords);

  json.SetBoolean("password", false);
  EXPECT_TRUE(ParseIncludePasswords(json, &include_passwords));
  EXPECT_FALSE(include_passwords);

  // Test against random string.
  json.SetString("password", "yes");
  EXPECT_FALSE(ParseIncludePasswords(json, &include_passwords));
}

// Checks that CredentialRequestOptions.mediation field is parsed
// correctly.
TEST_F(CredentialManagerUtilTest, ParseMediationRequirement) {
  base::DictionaryValue json;
  CredentialMediationRequirement mediation;

  // Default value should be kOptional.
  EXPECT_TRUE(ParseMediationRequirement(json, &mediation));
  EXPECT_EQ(CredentialMediationRequirement::kOptional, mediation);

  // "silent"/"optional"/"required" values should be parsed correctly.
  json.SetString("mediation", "silent");
  EXPECT_TRUE(ParseMediationRequirement(json, &mediation));
  EXPECT_EQ(CredentialMediationRequirement::kSilent, mediation);

  json.SetString("mediation", "optional");
  EXPECT_TRUE(ParseMediationRequirement(json, &mediation));
  EXPECT_EQ(CredentialMediationRequirement::kOptional, mediation);

  json.SetString("mediation", "required");
  EXPECT_TRUE(ParseMediationRequirement(json, &mediation));
  EXPECT_EQ(CredentialMediationRequirement::kRequired, mediation);

  // Test against random string.
  json.SetString("mediation", "dksjl");
  EXPECT_FALSE(ParseMediationRequirement(json, &mediation));
}

// Checks that Credential.type field is parsed correctly.
TEST_F(CredentialManagerUtilTest, ParseCredentialType) {
  base::DictionaryValue json;
  CredentialType type;

  // JS object Credential must contain |type| field.
  EXPECT_FALSE(ParseCredentialType(json, &type));

  // "PasswordCredential"/"FederatedCredential" values should be parsed
  // correctly.
  json.SetString("type", "PasswordCredential");
  EXPECT_TRUE(ParseCredentialType(json, &type));
  EXPECT_EQ(CredentialType::CREDENTIAL_TYPE_PASSWORD, type);

  json.SetString("type", "FederatedCredential");
  EXPECT_TRUE(ParseCredentialType(json, &type));
  EXPECT_EQ(CredentialType::CREDENTIAL_TYPE_FEDERATED, type);

  // "Credential" is not a valid type.
  json.SetString("type", "Credential");
  EXPECT_FALSE(ParseCredentialType(json, &type));

  // Empty string is also not allowed.
  json.SetString("type", "");
  EXPECT_FALSE(ParseCredentialType(json, &type));
}

// Checks that common fields of PasswordCredential and FederatedCredential are
// parsed correctly.
TEST_F(CredentialManagerUtilTest, ParseCommonCredentialFields) {
  // Building PasswordCredential because ParseCredentialDictionary for
  // Credential containing only common fields would return false.
  base::DictionaryValue json = BuildExampleValidPasswordCredential();
  CredentialInfo credential;

  // Valid dictionary should be parsed correctly and ParseCredentialDictionary
  // should return true.
  EXPECT_TRUE(ParseCredentialDictionary(json, &credential));
  EXPECT_EQ(base::ASCIIToUTF16("john@doe.com"), credential.id);
  EXPECT_EQ(base::ASCIIToUTF16("John Doe"), credential.name);
  EXPECT_EQ(GURL(kTestIconURL), credential.icon);

  // |id| field is required.
  json.Remove("id", nullptr);
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));

  // |iconURL| field is not required.
  json = BuildExampleValidPasswordCredential();
  json.Remove("iconURL", nullptr);
  EXPECT_TRUE(ParseCredentialDictionary(json, &credential));

  // If Credential has |iconURL| field, it must be a valid URL.
  json.SetString("iconURL", "not a valid url");
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));

  // If Credential has |iconURL| field, it must be a secure URL.
  json.SetString("iconURL", "http://example.com");
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));

  // Check that empty |iconURL| field is treated as no |iconURL| field.
  json.SetString("iconURL", "");
  EXPECT_TRUE(ParseCredentialDictionary(json, &credential));
}

// Checks that |password| and |type| fields of PasswordCredential are parsed
// correctly.
TEST_F(CredentialManagerUtilTest, ParsePasswordCredential) {
  base::DictionaryValue json = BuildExampleValidPasswordCredential();
  CredentialInfo credential;

  // Valid dictionary should be parsed correctly and ParseCredentialDictionary
  // should return true.
  EXPECT_TRUE(ParseCredentialDictionary(json, &credential));
  EXPECT_EQ(CredentialType::CREDENTIAL_TYPE_PASSWORD, credential.type);
  EXPECT_EQ(base::ASCIIToUTF16("admin123"), credential.password);

  // |password| field is required.
  json.Remove("password", nullptr);
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));

  // |password| field is cannot be an empty string.
  json.SetString("password", "");
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));
}

// Checks that |provider| and |type| fields of FederatedCredential are parsed
// correctly.
TEST_F(CredentialManagerUtilTest, ParseFederatedCredential) {
  base::DictionaryValue json = BuildExampleValidFederatedCredential();
  CredentialInfo credential;

  // Valid dictionary should be parsed correctly and ParseCredentialDictionary
  // should return true.
  EXPECT_TRUE(ParseCredentialDictionary(json, &credential));
  EXPECT_EQ(CredentialType::CREDENTIAL_TYPE_FEDERATED, credential.type);
  EXPECT_EQ(GURL(kTestWebOrigin), credential.federation.GetURL());

  // |provider| field is required.
  json.Remove("provider", nullptr);
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));

  // |provider| field cannot be an empty string.
  json.SetString("provider", "");
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));

  // |provider| field must be a valid URL.
  json.SetString("provider", "not a valid URL");
  EXPECT_FALSE(ParseCredentialDictionary(json, &credential));
}

// Checks that |providers| field of FederatedCredentialRequestOptions is
// parsed correctly.
TEST_F(CredentialManagerUtilTest, ParseFederations) {
  base::DictionaryValue json;

  // Build example valid |providers| list.
  std::unique_ptr<base::ListValue> list_ptr =
      base::MakeUnique<base::ListValue>();
  list_ptr->GetList().emplace_back(kTestWebOrigin);
  list_ptr->GetList().emplace_back("https://google.com");
  json.SetList("providers", std::move(list_ptr));
  std::vector<GURL> federations;

  // Check that parsing valid |providers| results in correct |federations| list.
  EXPECT_TRUE(ParseFederations(json, &federations));
  EXPECT_THAT(federations, testing::ElementsAre(GURL(kTestWebOrigin),
                                                GURL("https://google.com")));

  // ParseFederations should skip invalid URLs.
  list_ptr = base::MakeUnique<base::ListValue>();
  list_ptr->GetList().emplace_back(kTestWebOrigin);
  list_ptr->GetList().emplace_back("not a valid url");
  json.SetList("providers", std::move(list_ptr));
  EXPECT_TRUE(ParseFederations(json, &federations));
  EXPECT_THAT(federations, testing::ElementsAre(GURL(kTestWebOrigin)));

  // If |providers| is not a valid list, ParseFederations should return false.
  json.SetString("providers", kTestWebOrigin);
  EXPECT_FALSE(ParseFederations(json, &federations));
}
