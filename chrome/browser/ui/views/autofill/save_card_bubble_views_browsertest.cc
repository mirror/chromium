// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views_browsertest_base.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/window/dialog_client_view.h"

namespace autofill {

class SaveCardBubbleViewsFullFormBrowserTest
    : public SaveCardBubbleViewsBrowserTestBase {
 protected:
  SaveCardBubbleViewsFullFormBrowserTest()
      : SaveCardBubbleViewsBrowserTestBase(
            "/credit_card_upload_form_address_and_cc.html") {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleViewsFullFormBrowserTest);
};

class SaveCardBubbleViewsFullFormWithShippingBrowserTest
    : public SaveCardBubbleViewsBrowserTestBase {
 protected:
  SaveCardBubbleViewsFullFormWithShippingBrowserTest()
      : SaveCardBubbleViewsBrowserTestBase(
            "/credit_card_upload_form_shipping_address.html") {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleViewsFullFormWithShippingBrowserTest);
};


// Local test: Works but warns about URLRequestContextGetter leak


IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ClickingSaveClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  ResetEventObserverForSequence({DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [Save] should accept and close it.
  ResetEventObserverForSequence(
      {DialogEvent::BUBBLE_ACCEPTED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}


// Upload test: Test itself passes; fails due to leak


IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingSaveClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_SHOWN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [Save] should accept and close it, then send an UploadCardRequest
  // to Google Payments.
  ResetEventObserverForSequence(
      {DialogEvent::BUBBLE_ACCEPTED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}


// Upload test alternate: Test itself passes; fails due to leak


IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_Entering3DigitCvcAndClickingConfirmClosesBubbleIfNoCvcAndCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_HIDDEN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();

  // Clicking [Next] should not close the bubble, but rather advance to the
  // request CVC step and show the legal message footer.
  ResetEventObserverForSequence(
      {DialogEvent::CVC_REQUEST_VIEW_SHOWN, DialogEvent::FOOTER_SHOWN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);

  // Clicking [Confirm] after entering CVC should accept and close the bubble.
  ResetEventObserverForSequence({DialogEvent::BUBBLE_ACCEPTED,
                                 DialogEvent::CVC_REQUEST_VIEW_ACCEPTED,
                                 DialogEvent::BUBBLE_CLOSED});
  views::Textfield* cvc_field = static_cast<views::Textfield*>(
      FindViewInBubbleById(DialogViewId::CVC_TEXTFIELD));
  cvc_field->InsertOrReplaceText(base::ASCIIToUTF16("123"));
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

}  // namespace autofill
