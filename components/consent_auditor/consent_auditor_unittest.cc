// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/consent_auditor/consent_auditor.h"

#include <memory>

#include "components/consent_auditor/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/user_events/fake_user_event_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace consent_auditor {

namespace {

const char kLocalConsentDescriptionKey[] = "description";
const char kLocalConsentConfirmationKey[] = "confirmation";

}  // namespace

class ConsentAuditorTest : public testing::Test {
 public:
  void SetUp() override {
    pref_service_ = base::MakeUnique<TestingPrefServiceSimple>();
    user_event_service_ = base::MakeUnique<syncer::FakeUserEventService>();
    consent_auditor_ = base::MakeUnique<ConsentAuditor>(
        pref_service_.get(), user_event_service_.get());
    ConsentAuditor::RegisterPrefs(pref_service_->registry());
  }

  ConsentAuditor* consent_auditor() { return consent_auditor_.get(); }

  PrefService* pref_service() const { return pref_service_.get(); }

  syncer::UserEventService* user_event_service() const {
    return user_event_service_.get();
  }

 private:
  std::unique_ptr<ConsentAuditor> consent_auditor_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<syncer::FakeUserEventService> user_event_service_;
};

TEST_F(ConsentAuditorTest, LocalConsentPrefRepresentation) {
  // No consents are written at first.
  EXPECT_FALSE(pref_service()->HasPrefPath(prefs::kLocalConsentsDictionary));

  // Record a consent and check that it appears in the prefs.
  const std::string kFeature1Description = "This will enable feature 1.";
  const std::string kFeature1Confirmation = "OK.";
  EXPECT_TRUE(consent_auditor()->RecordLocalConsent(
      "feature1", kFeature1Description, kFeature1Confirmation));
  ASSERT_TRUE(pref_service()->HasPrefPath(prefs::kLocalConsentsDictionary));
  const base::DictionaryValue* consents =
      pref_service()->GetDictionary(prefs::kLocalConsentsDictionary);
  ASSERT_TRUE(consents);
  const base::DictionaryValue* record =
      static_cast<const base::DictionaryValue*>(
          consents->FindKeyOfType("feature1", base::Value::Type::DICTIONARY));
  ASSERT_TRUE(record);
  const base::Value* description = record->FindKey(kLocalConsentDescriptionKey);
  const base::Value* confirmation =
      record->FindKey(kLocalConsentConfirmationKey);
  ASSERT_TRUE(description && description->is_string());
  ASSERT_TRUE(confirmation && confirmation->is_string());
  EXPECT_EQ(description->GetString(), kFeature1Description);
  EXPECT_EQ(confirmation->GetString(), kFeature1Confirmation);

  // Do the same for another feature.
  const std::string kFeature2Description = "Enable feature 2?";
  const std::string kFeature2Confirmation = "Yes.";
  EXPECT_TRUE(consent_auditor()->RecordLocalConsent(
      "feature2", kFeature2Description, kFeature2Confirmation));
  record = static_cast<const base::DictionaryValue*>(
      consents->FindKeyOfType("feature2", base::Value::Type::DICTIONARY));
  ASSERT_TRUE(record);
  description = record->FindKey(kLocalConsentDescriptionKey);
  confirmation = record->FindKey(kLocalConsentConfirmationKey);
  ASSERT_TRUE(description && description->is_string());
  ASSERT_TRUE(confirmation && confirmation->is_string());
  EXPECT_EQ(description->GetString(), kFeature2Description);
  EXPECT_EQ(confirmation->GetString(), kFeature2Confirmation);

  // They are two separate records; the latter did not overwrite the former.
  EXPECT_EQ(2u, consents->size());
  EXPECT_TRUE(
      consents->FindKeyOfType("feature1", base::Value::Type::DICTIONARY));

  // Overwrite an existing consent.
  const std::string kFeature2NewDescription = "Re-enable feature 2?";
  const std::string kFeature2NewConfirmation = "Yes again.";
  EXPECT_TRUE(consent_auditor()->RecordLocalConsent(
      "feature2", kFeature2NewDescription, kFeature2NewConfirmation));
  record = static_cast<const base::DictionaryValue*>(
      consents->FindKeyOfType("feature2", base::Value::Type::DICTIONARY));
  ASSERT_TRUE(record);
  description = record->FindKey(kLocalConsentDescriptionKey);
  confirmation = record->FindKey(kLocalConsentConfirmationKey);
  ASSERT_TRUE(description && description->is_string());
  ASSERT_TRUE(confirmation && confirmation->is_string());
  EXPECT_EQ(description->GetString(), kFeature2NewDescription);
  EXPECT_EQ(confirmation->GetString(), kFeature2NewConfirmation);

  // We still have two records.
  EXPECT_EQ(2u, consents->size());
}

}  // namespace consent_auditor
