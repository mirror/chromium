// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/autofill_agent.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

TEST(AutofillAgentTest, GetMultipleValueFromSuggestion) {
  // Test filling an empty "multiple" field with "Suggestion".
  EXPECT_EQ(base::ASCIIToUTF16("Suggestion"),
            AutofillAgent::GetMultipleValueFromSuggestion(
                base::ASCIIToUTF16(""), base::ASCIIToUTF16("Suggestion")));

  // Test filling a "multiple" field with a single item. All text should be
  // replaced.
  EXPECT_EQ(
      base::ASCIIToUTF16("Suggestion"),
      AutofillAgent::GetMultipleValueFromSuggestion(
          base::ASCIIToUTF16("OldText"), base::ASCIIToUTF16("Suggestion")));

  // Test filling a "multiple" field with several comma-separated items. Only
  // the last item should be replaced.
  EXPECT_EQ(base::ASCIIToUTF16("OldText,Suggestion"),
            AutofillAgent::GetMultipleValueFromSuggestion(
                base::ASCIIToUTF16("OldText,OtherText"),
                base::ASCIIToUTF16("Suggestion")));

  // Test filling a "multiple" field with several comma-separated items, and a
  // trailing comma. The suggestion should be added after the last comma.
  EXPECT_EQ(base::ASCIIToUTF16("OldText,OtherText,Suggestion"),
            AutofillAgent::GetMultipleValueFromSuggestion(
                base::ASCIIToUTF16("OldText,OtherText,"),
                base::ASCIIToUTF16("Suggestion")));
}

}  // namespace autofill.
