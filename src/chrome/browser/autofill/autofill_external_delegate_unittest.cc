// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/string16.h"
#include "chrome/browser/autofill/autofill_manager.h"
#include "chrome/browser/autofill/test_autofill_external_delegate.h"
#include "chrome/browser/ui/tab_contents/test_tab_contents_wrapper.h"
#include "chrome/test/base/testing_profile.h"
#include "content/test/test_browser_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/rect.h"
#include "webkit/forms/form_data.h"
#include "webkit/forms/form_field.h"

using content::BrowserThread;
using testing::_;
using webkit::forms::FormData;
using webkit::forms::FormField;

namespace {

class MockAutofillExternalDelegate : public TestAutofillExternalDelegate {
 public:
  MockAutofillExternalDelegate(TabContentsWrapper* wrapper,
                               AutofillManager* autofill_manger)
      : TestAutofillExternalDelegate(wrapper, autofill_manger) {}
  ~MockAutofillExternalDelegate() {}

  MOCK_METHOD5(ApplyAutofillSuggestions, void(
      const std::vector<string16>& autofill_values,
      const std::vector<string16>& autofill_labels,
      const std::vector<string16>& autofill_icons,
      const std::vector<int>& autofill_unique_ids,
      int separator_index));

  MOCK_METHOD4(OnQueryPlatformSpecific,
               void(int query_id,
                    const webkit::forms::FormData& form,
                    const webkit::forms::FormField& field,
                    const gfx::Rect& bounds));

  MOCK_METHOD0(HideAutofillPopup, void());

 private:
  virtual void HideAutofillPopupInternal() {};
};

class MockAutofillManager : public AutofillManager {
 public:
  explicit MockAutofillManager(TabContentsWrapper* tab_contents)
      : AutofillManager(tab_contents) {}
  ~MockAutofillManager() {}

  MOCK_METHOD4(OnFillAutofillFormData,
               void(int query_id,
                    const webkit::forms::FormData& form,
                    const webkit::forms::FormField& field,
                    int unique_id));
};

}  // namespace

class AutofillExternalDelegateUnitTest : public TabContentsWrapperTestHarness {
 public:
  AutofillExternalDelegateUnitTest()
      : ui_thread_(BrowserThread::UI, &message_loop_) {}
  virtual ~AutofillExternalDelegateUnitTest() {}

  virtual void SetUp() OVERRIDE {
    TabContentsWrapperTestHarness::SetUp();
    autofill_manager_ = new MockAutofillManager(contents_wrapper());
    external_delegate_.reset(new MockAutofillExternalDelegate(
        contents_wrapper(),
        autofill_manager_));
  }

 protected:
  scoped_refptr<MockAutofillManager> autofill_manager_;
  scoped_ptr<MockAutofillExternalDelegate> external_delegate_;

 private:
  content::TestBrowserThread ui_thread_;

  DISALLOW_COPY_AND_ASSIGN(AutofillExternalDelegateUnitTest);
};

// Test that our external delegate called the virtual methods at the right time.
TEST_F(AutofillExternalDelegateUnitTest, TestExternalDelegateVirtualCalls) {
  const int kQueryId = 5;
  const FormData form;
  FormField field;
  field.is_focusable = true;
  field.should_autocomplete = true;
  const gfx::Rect bounds;

  EXPECT_CALL(*external_delegate_,
              OnQueryPlatformSpecific(kQueryId, form, field, bounds));

  // This should call OnQueryPlatform specific.
  external_delegate_->OnQuery(kQueryId, form, field, bounds, false);

  EXPECT_CALL(*external_delegate_, ApplyAutofillSuggestions(_, _, _, _, _));

  // This should call ApplyAutofillSuggestions.
  std::vector<string16> autofill_item;
  autofill_item.push_back(string16());
  std::vector<int> autofill_ids;
  autofill_ids.push_back(1);
  external_delegate_->OnSuggestionsReturned(kQueryId,
                                            autofill_item,
                                            autofill_item,
                                            autofill_item,
                                            autofill_ids);


  EXPECT_CALL(*external_delegate_, HideAutofillPopup());

  // This should trigger a call to hide the popup since
  // we've selected an option.
  external_delegate_->DidAcceptAutofillSuggestions(autofill_item[0],
                                                   autofill_ids[0], 0);
}

// Test that the Autofill delegate doesn't try and fill a form with a
// negative unique id.
TEST_F(AutofillExternalDelegateUnitTest, ExternalDelegateInvalidUniqueId) {
  // Ensure it doesn't try to preview the negative id.
  EXPECT_CALL(*autofill_manager_, OnFillAutofillFormData(_, _, _, _)).Times(0);
  external_delegate_->SelectAutofillSuggestionAtIndex(-1, 0);

  // Ensure it doesn't try to fill the form in with the negative id.
  EXPECT_CALL(*autofill_manager_, OnFillAutofillFormData(_, _, _, _)).Times(0);
  external_delegate_->DidAcceptAutofillSuggestions(string16(), -1, 0);
}
