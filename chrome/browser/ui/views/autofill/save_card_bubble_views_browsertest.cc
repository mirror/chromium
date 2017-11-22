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

// === Local credit card save bubble tests

IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Local_SubmittingFormShowsBubbleIfGooglePaymentsDeclines) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  ResetEventObserverForSequence({DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Local_SubmittingFormShowsBubbleIfGetUploadDetailsRpcFails) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcServerError();

  // Submitting the form and having the call to Payments fail should show the
  // local save bubble.
  ResetEventObserverForSequence({DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

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

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ClickingNoThanksClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  ResetEventObserverForSequence({DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [No thanks] should cancel and close it.
  ResetEventObserverForSequence(
      {DialogEvent::BUBBLE_CANCELLED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::CANCEL_BUTTON);
}

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ClickingLearnMoreClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  ResetEventObserverForSequence({DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking the [Learn more] link should hide the bubble.
  // (Not preferred behavior, but it's what we've got.)
  // TODO(jsaul): Is there a way to assert the [Learn more] link opens its
  //              corresponding Chrome Help page?
  ResetEventObserverForSequence(
      {DialogEvent::LEARN_MORE_LINK_CLICKED, DialogEvent::BUBBLE_HIDDEN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::LEARN_MORE_LINK);
}

// === Upload credit card save bubble tests

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_SubmittingFormShowsBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_SHOWN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// TODO(jsaul): Is there a way to assert an UploadCardRequest was sent at the
//              end of the test?
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingSaveClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_SHOWN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [Save] should accept and close it.
  ResetEventObserverForSequence(
      {DialogEvent::BUBBLE_ACCEPTED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingNoThanksClosesBubbleIfNewUiExpOff) {
  DisableAutofillUpstreamShowNewUiExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_SHOWN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [No thanks] should cancel and close it.
  ResetEventObserverForSequence(
      {DialogEvent::BUBBLE_CANCELLED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::CANCEL_BUTTON);
}

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ShouldNotHaveNoThanksButtonIfNewUiExperimentOn) {
  EnableAutofillUpstreamShowNewUiExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_SHOWN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Assert that the cancel button cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::CANCEL_BUTTON));
}

// TODO(jsaul): There doesn't seem to be a good way of accessing and clicking
//              the [X] close button in the top-right corner of the dialog.
//              Calling Close() on the bubble doesn't reach WindowClosing().
//              The closest thing would be forcibly destroying the bubble, but
//              at that point the test is already far removed from emulating
//              clicking the [X]. When that can be worked around, create a
//              Upload_ClickingCloseClosesBubble test.

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingLearnMoreClosesBubbleIfNewUiExpOff) {
  DisableAutofillUpstreamShowNewUiExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_SHOWN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking the [Learn more] link should hide the bubble.
  // (Not preferred behavior, but it's what we've got.)
  // TODO(jsaul): Is there a way to assert the [Learn more] link opens its
  //              corresponding Chrome Help page?
  ResetEventObserverForSequence(
      {DialogEvent::LEARN_MORE_LINK_CLICKED, DialogEvent::BUBBLE_HIDDEN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::LEARN_MORE_LINK);
}

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ShouldNotHaveLearnMoreLinkIfNewUiExperimentOn) {
  EnableAutofillUpstreamShowNewUiExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_SHOWN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Assert that the Learn more link cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::LEARN_MORE_LINK));
}

// TODO(jsaul): Only *part* of the legal message StyledLabel is clickable, and
//              the NOTREACHED() in SaveCardBubbleViews::StyledLabelLinkClicked
//              prevents us from being able to click it unless we know the exact
//              range of the link. When that can be worked around, create a
//              Upload_ClickingTosLinkClosesBubble test.

IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_SubmittingFormWithoutCvcShowsBubbleIfCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_HIDDEN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();
}

IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_SubmittingFormWithInvalidCvcShowsBubbleIfCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_HIDDEN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithInvalidCvc();
  WaitForObservedEvent();
}

IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_ClickingNextDoesNotCloseBubbleIfNoCvcAndCvcExpOn) {
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
}

IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_ClickingDisabledConfirmDoesNotCloseBubbleIfNoCvcAndCvcExpOn) {
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

  // Clicking [Confirm] should be impossible because the button is disabled.
  ResetEventObserverForSequence({});
  // TODO(jsaul): "ResetEventObserverForSequence({})" doesn't work as intended,
  //              as it just returns immediately and *then* events could occur.
  //              Find a way to assert no further events happened upon click.
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

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

IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_Entering4DigitCvcAndClickingConfirmClosesBubbleIfNoCvcAndCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventObserverForSequence(
      {DialogEvent::FOOTER_HIDDEN, DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithAmexWithoutCvc();
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
  cvc_field->InsertOrReplaceText(base::ASCIIToUTF16("1234"));
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

// === Deny offer-to-save logic tests

// TODO(jsaul): Complete these tests before submitting the CL.  They need to be
//              able to fill different parts of the form (as well as a new form
//              entirely to test conflicting addresses).
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfCardNumberNotFound) {}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfInvalidCardNumberFound) {}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfExpirationDateNotFound) {}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfExpiredExpirationDateFound) {
}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfCvcNotFoundAndCvcExpOff) {}
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_ShouldNotOfferToSaveIfInvalidCvcFoundAndCvcExpOff) {}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfNameNotFound) {}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfNamesConflict) {}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfAddressNotFound) {}
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfPostalCodesConflict) {}

}  // namespace autofill
