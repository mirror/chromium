// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views_browsertest_base.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "ui/views/controls/button/label_button.h"
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

// Note: Tests using this class will need to use a TestNavigationObserver to
//       wait for the navigation between shipping address and billing address to
//       finish.
class SaveCardBubbleViewsFullFormWithShippingBrowserTest
    : public SaveCardBubbleViewsBrowserTestBase {
 protected:
  SaveCardBubbleViewsFullFormWithShippingBrowserTest()
      : SaveCardBubbleViewsBrowserTestBase(
            "/credit_card_upload_form_shipping_address.html") {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleViewsFullFormWithShippingBrowserTest);
};

// Tests the local save bubble. Ensures that local save appears if the RPC to
// Google Payments fails unexpectedly.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Local_SubmittingFormShowsBubbleIfGetUploadDetailsRpcFails) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcServerError();

  // Submitting the form and having the call to Payments fail should show the
  // local save bubble.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::OFFERED_LOCAL_SAVE,
                               DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the local save bubble. Ensures that clicking the [Save] button
// successfully causes the bubble to go away.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ClickingSaveClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::OFFERED_LOCAL_SAVE,
                               DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [Save] should accept and close it.
  ResetEventWaiterForSequence(
      {DialogEvent::BUBBLE_ACCEPTED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

// Tests the local save bubble. Ensures that clicking the [No thanks] button
// successfully causes the bubble to go away.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Local_ClickingNoThanksClosesBubbleIfSecondaryUiMdExpOff) {
  DisableSecondaryUiMdExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::OFFERED_LOCAL_SAVE,
                               DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [No thanks] should cancel and close it.
  ResetEventWaiterForSequence(
      {DialogEvent::BUBBLE_CANCELLED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::CANCEL_BUTTON);
}

// Tests the local save bubble. Ensures that the Harmony version of the bubble
// does not have a [No thanks] button (it has an [X] Close button instead.)
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ShouldNotHaveNoThanksButtonIfSecondaryUiMdExpOn) {
  EnableSecondaryUiMdExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::OFFERED_LOCAL_SAVE,
                               DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Assert that the cancel button cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::CANCEL_BUTTON));
}

// Tests the local save bubble. Ensures that clicking the [Learn more] link
// causes the bubble to go away and opens the relevant help page.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Local_ClickingLearnMoreClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsDeclines();

  // Submitting the form and having Payments decline offering to save should
  // show the local save bubble.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::OFFERED_LOCAL_SAVE,
                               DialogEvent::BUBBLE_SHOWN_LOCAL});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking the [Learn more] link should hide the bubble and spawn a new tab.
  // (Hiding the bubble is not preferred behavior, but it's what we've got.)
  content::WebContentsAddedObserver web_contents_added_observer;
  ResetEventWaiterForSequence(
      {DialogEvent::LEARN_MORE_LINK_CLICKED, DialogEvent::BUBBLE_HIDDEN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::LEARN_MORE_LINK);
  content::WebContents* new_tab_contents =
      web_contents_added_observer.GetWebContents();
  EXPECT_EQ("https://support.google.com/chrome/?p=settings_autofill",
            new_tab_contents->GetVisibleURL().spec());
}

// Tests the upload save bubble. Ensures that clicking the [Save] button
// successfully causes the bubble to go away and sends an UploadCardRequest RPC
// to Google Payments.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingSaveClosesBubble) {
  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_SHOWN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [Save] should accept and close it, then send an UploadCardRequest
  // to Google Payments.
  ResetEventWaiterForSequence({DialogEvent::BUBBLE_ACCEPTED,
                               DialogEvent::BUBBLE_CLOSED,
                               DialogEvent::SENT_UPLOAD_CARD_REQUEST});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

// Tests the upload save bubble. Ensures that clicking the [No thanks] button
// successfully causes the bubble to go away.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_ClickingNoThanksClosesBubbleIfSecondaryUiMdExpOff) {
  DisableSecondaryUiMdExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_SHOWN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking [No thanks] should cancel and close it.
  ResetEventWaiterForSequence(
      {DialogEvent::BUBBLE_CANCELLED, DialogEvent::BUBBLE_CLOSED});
  ClickOnDialogViewWithIdAndWait(DialogViewId::CANCEL_BUTTON);
}

// Tests the upload save bubble. Ensures that the Harmony version of the bubble
// does not have a [No thanks] button (it has an [X] Close button instead.)
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ShouldNotHaveNoThanksButtonIfSecondaryUiMdExpOn) {
  EnableSecondaryUiMdExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_SHOWN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Assert that the cancel button cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::CANCEL_BUTTON));
}

// TODO(crbug.com/791861): There doesn't seem to be a good way of accessing and
// clicking the [X] close button in the top-right corner of the dialog. Calling
// Close() on the bubble doesn't reach WindowClosing(). The closest thing would
// be forcibly destroying the bubble, but at that point the test is already far
// removed from emulating clicking the [X]. When/if that can be worked around,
// create an Upload_ClickingCloseClosesBubbleIfSecondaryUiMdExpOn test.

// Tests the upload save bubble. Ensures that clicking the [Learn more] link
// causes the bubble to go away and opens the relevant help page.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ClickingLearnMoreClosesBubbleIfNewUiExpOff) {
  DisableAutofillUpstreamShowNewUiExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_SHOWN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Clicking the [Learn more] link should hide the bubble and spawn a new tab.
  // (Hiding the bubble is not preferred behavior, but it's what we've got.)
  content::WebContentsAddedObserver web_contents_added_observer;
  ResetEventWaiterForSequence(
      {DialogEvent::LEARN_MORE_LINK_CLICKED, DialogEvent::BUBBLE_HIDDEN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::LEARN_MORE_LINK);
  content::WebContents* new_tab_contents =
      web_contents_added_observer.GetWebContents();
  EXPECT_EQ("https://support.google.com/chrome/?p=settings_autofill",
            new_tab_contents->GetVisibleURL().spec());
}

// Tests the upload save bubble. Ensures that the updated version of the bubble
// does not have a [Learn more] link.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ShouldNotHaveLearnMoreLinkIfNewUiExperimentOn) {
  EnableAutofillUpstreamShowNewUiExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble and legal footer.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_SHOWN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitForm();
  WaitForObservedEvent();

  // Assert that the Learn more link cannot be found.
  EXPECT_FALSE(FindViewInBubbleById(DialogViewId::LEARN_MORE_LINK));
}

// TODO(jsaul): Only *part* of the legal message StyledLabel is clickable, and
//              the NOTREACHED() in SaveCardBubbleViews::StyledLabelLinkClicked
//              prevents us from being able to click it unless we know the exact
//              gfx::Range of the link. When/if that can be worked around,
//              create an Upload_ClickingTosLinkClosesBubble test.

// Tests the upload save bubble. Ensures that if CVC is invalid when the credit
// card is submitted, the bubble is still shown with the CVC fix flow (starts
// with the footer hidden due to showing it on the second step).
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_SubmittingFormWithInvalidCvcShowsBubbleIfCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_HIDDEN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithInvalidCvc();
  WaitForObservedEvent();
}

// Tests the upload save bubble. Ensures that during the CVC fix flow, the final
// [Confirm] button is disabled if CVC has not yet been entered.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ConfirmButtonIsDisabledIfNoCvcAndCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_HIDDEN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();

  // Clicking [Next] should not close the bubble, but rather advance to the
  // request CVC step and show the legal message footer.
  ResetEventWaiterForSequence(
      {DialogEvent::CVC_REQUEST_VIEW_SHOWN, DialogEvent::FOOTER_SHOWN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);

  // The [Confirm] button should be disabled due to no CVC yet entered.
  views::LabelButton* confirm_button = static_cast<views::LabelButton*>(
      FindViewInBubbleById(DialogViewId::OK_BUTTON));
  EXPECT_EQ(confirm_button->state(),
            views::LabelButton::ButtonState::STATE_DISABLED);
}

// Tests the upload save bubble. Ensures that during the CVC fix flow, the final
// [Confirm] button is disabled if the entered CVC is invalid.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Upload_ConfirmButtonIsDisabledIfInvalidCvcAndCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_HIDDEN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();

  // Clicking [Next] should not close the bubble, but rather advance to the
  // request CVC step and show the legal message footer.
  ResetEventWaiterForSequence(
      {DialogEvent::CVC_REQUEST_VIEW_SHOWN, DialogEvent::FOOTER_SHOWN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);

  // Enter an invalid length CVC (4-digit CVC for non-AmEx card, for instance).
  views::Textfield* cvc_field = static_cast<views::Textfield*>(
      FindViewInBubbleById(DialogViewId::CVC_TEXTFIELD));
  cvc_field->InsertOrReplaceText(base::ASCIIToUTF16("1234"));

  // The [Confirm] button should be disabled due to the invalid CVC.
  views::LabelButton* confirm_button = static_cast<views::LabelButton*>(
      FindViewInBubbleById(DialogViewId::OK_BUTTON));
  EXPECT_EQ(confirm_button->state(),
            views::LabelButton::ButtonState::STATE_DISABLED);
}

// Tests the upload save bubble. Ensures that during the CVC fix flow, if a
// valid 3-digit CVC is entered, the [Confirm] button successfully causes the
// bubble to go away and sends an UploadCardRequest RPC to Google Payments.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_Entering3DigitCvcAndClickingConfirmClosesBubbleIfNoCvcAndCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_HIDDEN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();

  // Clicking [Next] should not close the bubble, but rather advance to the
  // request CVC step and show the legal message footer.
  ResetEventWaiterForSequence(
      {DialogEvent::CVC_REQUEST_VIEW_SHOWN, DialogEvent::FOOTER_SHOWN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);

  // Clicking [Confirm] after entering CVC should accept and close the bubble,
  // then send an UploadCardRequest to Google Payments.
  ResetEventWaiterForSequence(
      {DialogEvent::BUBBLE_ACCEPTED, DialogEvent::CVC_REQUEST_VIEW_ACCEPTED,
       DialogEvent::BUBBLE_CLOSED, DialogEvent::SENT_UPLOAD_CARD_REQUEST});
  views::Textfield* cvc_field = static_cast<views::Textfield*>(
      FindViewInBubbleById(DialogViewId::CVC_TEXTFIELD));
  cvc_field->InsertOrReplaceText(base::ASCIIToUTF16("123"));
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

// Tests the upload save bubble. Ensures that during the CVC fix flow, if a
// valid 4-digit CVC (for an AmEx card) is entered, the [Confirm] button
// successfully causes the bubble to go away and sends an UploadCardRequest RPC
// to Google Payments.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Upload_Entering4DigitCvcAndClickingConfirmClosesBubbleIfNoCvcAndCvcExpOn) {
  EnableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Set up the Payments RPC.
  SetUploadDetailsRpcPaymentsAccepts();

  // Submitting the form should show the upload save bubble, but should NOT show
  // the legal footer on the initial step.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE,
                               DialogEvent::FOOTER_HIDDEN,
                               DialogEvent::BUBBLE_SHOWN_UPLOAD});
  FillAndSubmitFormWithAmexWithoutCvc();
  WaitForObservedEvent();

  // Clicking [Next] should not close the bubble, but rather advance to the
  // request CVC step and show the legal message footer.
  ResetEventWaiterForSequence(
      {DialogEvent::CVC_REQUEST_VIEW_SHOWN, DialogEvent::FOOTER_SHOWN});
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);

  // Clicking [Confirm] after entering CVC should accept and close the bubble,
  // then send an UploadCardRequest to Google Payments.
  ResetEventWaiterForSequence(
      {DialogEvent::BUBBLE_ACCEPTED, DialogEvent::CVC_REQUEST_VIEW_ACCEPTED,
       DialogEvent::BUBBLE_CLOSED, DialogEvent::SENT_UPLOAD_CARD_REQUEST});
  views::Textfield* cvc_field = static_cast<views::Textfield*>(
      FindViewInBubbleById(DialogViewId::CVC_TEXTFIELD));
  cvc_field->InsertOrReplaceText(base::ASCIIToUTF16("1234"));
  ClickOnDialogViewWithIdAndWait(DialogViewId::OK_BUTTON);
}

// Tests the upload save logic. Ensures that credit card upload is offered if
// name, address, and CVC are detected.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldAttemptToOfferToSaveIfEverythingFound) {
  // Submitting the form should start the flow of asking Payments if Chrome
  // should offer to save the card to Google.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is offered even
// if street addresses conflict, as long as their postal codes are the same.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormWithShippingBrowserTest,
    Logic_ShouldAttemptToOfferToSaveIfStreetAddressesConflict) {
  // Submit first shipping address form with a conflicting street address.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingStreetAddress();
  shipping_form_nav_observer.Wait();

  // Submitting the second form should start the flow of asking Payments if
  // Chrome should offer to save the Google, because conflicting street
  // addresses with the same postal code are allowable.
  ResetEventWaiterForSequence({DialogEvent::REQUESTED_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if CVC is not detected and the CVC fix flow is not enabled.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfCvcNotFoundAndCvcExpOff) {
  DisableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Submitting the form should not show the upload save bubble because CVC is
  // missing.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithoutCvc();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if an invalid CVC is detected and the CVC fix flow is not enabled.
IN_PROC_BROWSER_TEST_F(
    SaveCardBubbleViewsFullFormBrowserTest,
    Logic_ShouldNotOfferToSaveIfInvalidCvcFoundAndCvcExpOff) {
  DisableAutofillUpstreamRequestCvcIfMissingExperiment();

  // Submitting the form should not show the upload save bubble because CVC is
  // invalid.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithInvalidCvc();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if address/cardholder name is not detected.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfNameNotFound) {
  // Submitting the form should not show the upload save bubble because name is
  // missing.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithoutName();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if multiple conflicting names are detected.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormWithShippingBrowserTest,
                       Logic_ShouldNotOfferToSaveIfNamesConflict) {
  // Submit first shipping address form with a conflicting name.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingName();
  shipping_form_nav_observer.Wait();

  // Submitting the second form should not show the upload save bubble because
  // the name conflicts with the previous form.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if billing address is not detected.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormBrowserTest,
                       Logic_ShouldNotOfferToSaveIfAddressNotFound) {
  // Submitting the form should not show the upload save bubble because address
  // is missing.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitFormWithoutAddress();
  WaitForObservedEvent();
}

// Tests the upload save logic. Ensures that credit card upload is not offered
// if multiple conflicting billing address postal codes are detected.
IN_PROC_BROWSER_TEST_F(SaveCardBubbleViewsFullFormWithShippingBrowserTest,
                       Logic_ShouldNotOfferToSaveIfPostalCodesConflict) {
  // Submit first shipping address form with a conflicting postal code.
  content::TestNavigationObserver shipping_form_nav_observer(
      GetActiveWebContents(), 1);
  FillAndSubmitFormWithConflictingPostalCode();
  shipping_form_nav_observer.Wait();

  // Submitting the second form should not show the upload save bubble because
  // the postal code conflicts with the previous form.
  ResetEventWaiterForSequence({DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE});
  FillAndSubmitForm();
  WaitForObservedEvent();
}

}  // namespace autofill
