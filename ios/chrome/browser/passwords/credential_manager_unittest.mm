// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/credential_manager.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
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

class CredentialManagerTest : public PlatformTest {
 public:
  CredentialManagerTest() {}

  ~CredentialManagerTest() override {}

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
};

// Checks that CredentialRequestOptions.password field is parsed
// correctly.
TEST_F(CredentialManagerTest, ParseIncludePasswords) {
  base::DictionaryValue json;
  bool include_passwords;

  // Default value should be false.
  EXPECT_TRUE(
      CredentialManager::ParseIncludePasswords(json, include_passwords));
  EXPECT_FALSE(include_passwords);

  // true/false values should be parsed correctly.
  json.SetBoolean("password", true);
  EXPECT_TRUE(
      CredentialManager::ParseIncludePasswords(json, include_passwords));
  EXPECT_TRUE(include_passwords);

  json.SetBoolean("password", false);
  EXPECT_TRUE(
      CredentialManager::ParseIncludePasswords(json, include_passwords));
  EXPECT_FALSE(include_passwords);

  // Test against random string.
  json.SetString("password", "yes");
  EXPECT_FALSE(
      CredentialManager::ParseIncludePasswords(json, include_passwords));
}

// Checks that CredentialRequestOptions.mediation field is parsed
// correctly.
TEST_F(CredentialManagerTest, ParseMediationRequirement) {
  base::DictionaryValue json;
  CredentialMediationRequirement mediation;

  // Default value should be kOptional.
  EXPECT_TRUE(CredentialManager::ParseMediationRequirement(json, mediation));
  EXPECT_EQ(mediation, CredentialMediationRequirement::kOptional);

  // "silent"/"optional"/"required" values should be parsed correctly.
  json.SetString("mediation", "silent");
  EXPECT_TRUE(CredentialManager::ParseMediationRequirement(json, mediation));
  EXPECT_EQ(mediation, CredentialMediationRequirement::kSilent);

  json.SetString("mediation", "optional");
  EXPECT_TRUE(CredentialManager::ParseMediationRequirement(json, mediation));
  EXPECT_EQ(mediation, CredentialMediationRequirement::kOptional);

  json.SetString("mediation", "required");
  EXPECT_TRUE(CredentialManager::ParseMediationRequirement(json, mediation));
  EXPECT_EQ(mediation, CredentialMediationRequirement::kRequired);

  // Test against random string.
  json.SetString("mediation", "dksjl");
  EXPECT_FALSE(CredentialManager::ParseMediationRequirement(json, mediation));
}

// Checks that Credential.type field is parsed correctly.
TEST_F(CredentialManagerTest, ParseCredentialType) {
  base::DictionaryValue json;
  CredentialType type;

  // JS object Credential must contain |type| field.
  EXPECT_FALSE(CredentialManager::ParseCredentialType(json, type));

  // "PasswordCredential"/"FederatedCredential" values should be parsed
  // correctly.
  json.SetString("type", "PasswordCredential");
  EXPECT_TRUE(CredentialManager::ParseCredentialType(json, type));
  EXPECT_EQ(type, CredentialType::CREDENTIAL_TYPE_PASSWORD);

  json.SetString("type", "FederatedCredential");
  EXPECT_TRUE(CredentialManager::ParseCredentialType(json, type));
  EXPECT_EQ(type, CredentialType::CREDENTIAL_TYPE_FEDERATED);

  // "Credential" is not a valid type.
  json.SetString("type", "Credential");
  EXPECT_FALSE(CredentialManager::ParseCredentialType(json, type));

  // Empty string is also not allowed.
  json.SetString("type", "");
  EXPECT_FALSE(CredentialManager::ParseCredentialType(json, type));
}

// Checks that common fields of PasswordCredential and FederatedCredential are
// parsed correctly.
TEST_F(CredentialManagerTest, ParseCommonCredentialFields) {
  // Building PasswordCredential because ParseCredentialDictionary for
  // Credential containing only common fields would return false.
  base::DictionaryValue json = BuildExampleValidPasswordCredential();
  CredentialInfo credential;

  // Valid dictionary should be parsed correctly and ParseCredentialDictionary
  // should return true.
  EXPECT_TRUE(CredentialManager::ParseCredentialDictionary(json, credential));
  EXPECT_EQ(credential.id, base::ASCIIToUTF16("john@doe.com"));
  EXPECT_EQ(credential.name, base::ASCIIToUTF16("John Doe"));
  EXPECT_EQ(credential.icon, GURL(kTestIconURL));

  // |id| field is required.
  json.Remove("id", nullptr);
  EXPECT_FALSE(CredentialManager::ParseCredentialDictionary(json, credential));

  // |name| field is required.
  json = BuildExampleValidPasswordCredential();
  json.Remove("name", nullptr);
  EXPECT_FALSE(CredentialManager::ParseCredentialDictionary(json, credential));

  // |iconURL| field is not required.
  json = BuildExampleValidPasswordCredential();
  json.Remove("iconURL", nullptr);
  EXPECT_TRUE(CredentialManager::ParseCredentialDictionary(json, credential));

  // If Credential has |iconURL| field, it must be a valid URL.
  json.SetString("iconURL", "not a valid url");
  EXPECT_FALSE(CredentialManager::ParseCredentialDictionary(json, credential));

  // If Credential has |iconURL| field, it must be a secure URL.
  json.SetString("iconURL", "http://example.com");
  EXPECT_FALSE(CredentialManager::ParseCredentialDictionary(json, credential));

  // Check that empty |iconURL| field is treated as no |iconURL| field.
  json.SetString("iconURL", "");
  EXPECT_TRUE(CredentialManager::ParseCredentialDictionary(json, credential));
}

// Checks that |password| and |type| fields of PasswordCredential are parsed
// correctly.
TEST_F(CredentialManagerTest, ParsePasswordCredential) {
  base::DictionaryValue json = BuildExampleValidPasswordCredential();
  CredentialInfo credential;

  // Valid dictionary should be parsed correctly and ParseCredentialDictionary
  // should return true.
  EXPECT_TRUE(CredentialManager::ParseCredentialDictionary(json, credential));
  EXPECT_EQ(credential.type, CredentialType::CREDENTIAL_TYPE_PASSWORD);
  EXPECT_EQ(credential.password, base::ASCIIToUTF16("admin123"));

  // |password| field is required.
  json.Remove("password", nullptr);
  EXPECT_FALSE(CredentialManager::ParseCredentialDictionary(json, credential));
}

// Checks that |provider| and |type| fields of FederatedCredential are parsed
// correctly.
TEST_F(CredentialManagerTest, ParseFederatedCredential) {
  base::DictionaryValue json = BuildExampleValidFederatedCredential();
  CredentialInfo credential;

  // Valid dictionary should be parsed correctly and ParseCredentialDictionary
  // should return true.
  EXPECT_TRUE(CredentialManager::ParseCredentialDictionary(json, credential));
  EXPECT_EQ(credential.type, CredentialType::CREDENTIAL_TYPE_FEDERATED);
  EXPECT_EQ(credential.federation.scheme(), std::string("https"));
  EXPECT_EQ(credential.federation.host(), std::string("example.com"));

  // |federation| field is required.
  json.Remove("provider", nullptr);
  EXPECT_FALSE(CredentialManager::ParseCredentialDictionary(json, credential));

  // |provider| field must be a valid URL
  json.SetString("provider", "not a valid url");
  EXPECT_FALSE(CredentialManager::ParseCredentialDictionary(json, credential));
}

// Checks that |providers| field of FederatedCredentialRequestOptions is
// parsed correctly.
TEST_F(CredentialManagerTest, ParseFederations) {
  base::DictionaryValue json;

  // Build example valid |providers| list.
  std::unique_ptr<base::ListValue> list_ptr =
      base::MakeUnique<base::ListValue>();
  list_ptr->AppendString(kTestWebOrigin);
  list_ptr->AppendString("https://google.com");
  json.SetList("providers", std::move(list_ptr));
  std::vector<GURL> federations;

  // Check that parsing valid |providers| results in correct |federations| list.
  EXPECT_TRUE(CredentialManager::ParseFederations(json, federations));
  EXPECT_THAT(federations, testing::ElementsAre(GURL(kTestWebOrigin),
                                                GURL("https://google.com")));

  // If at least one element is invalid, ParseFederations should return false.
  list_ptr = base::MakeUnique<base::ListValue>();
  list_ptr->AppendString(kTestWebOrigin);
  list_ptr->AppendString("not a valid url");
  json.SetList("providers", std::move(list_ptr));
  EXPECT_FALSE(CredentialManager::ParseFederations(json, federations));

  // If |providers| is not a valid list, ParseFederations should return false.
  json.SetString("providers", kTestWebOrigin);
  EXPECT_FALSE(CredentialManager::ParseFederations(json, federations));
}
