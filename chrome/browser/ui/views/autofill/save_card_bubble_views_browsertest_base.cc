// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/save_card_bubble_views_browsertest_base.h"

#include <list>
#include <memory>
#include <string>

#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/autofill/dialog_event_observer.cc"
#include "chrome/browser/ui/views/autofill/save_card_bubble_view_ids.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/dialog_client_view.h"

namespace autofill {

namespace {

const char kURLGetUploadDetailsRequest[] =
    "https://payments.google.com/payments/apis/chromepaymentsservice/"
    "getdetailsforsavecard";
const char kResponseGetUploadDetailsSuccess[] =
    "{\"legal_message\":{\"line\":[{\"template\":\"Legal message template with "
    "link: "
    "{0}.\",\"template_parameter\":[{\"display_text\":\"Link\",\"url\":\"https:"
    "//www.example.com/\"}]}]},\"context_token\":\"dummy_context_token\"}";
const char kResponseGetUploadDetailsFailure[] =
    "{\"error\":{\"code\":\"FAILED_PRECONDITION\",\"user_error_message\":\"An "
    "unexpected error has occurred. Please try again later.\"}}";

}  // namespace

SaveCardBubbleViewsBrowserTestBase::SaveCardBubbleViewsBrowserTestBase(
    const std::string& test_file_path)
    : test_file_path_(test_file_path) {}

SaveCardBubbleViewsBrowserTestBase::~SaveCardBubbleViewsBrowserTestBase() {}

void SaveCardBubbleViewsBrowserTestBase::SetUpCommandLine(
    base::CommandLine* command_line) {
  // HTTPS server only serves a valid cert for localhost, so this is needed to
  // load pages from "a.com" without an interstitial.
  command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
}

void SaveCardBubbleViewsBrowserTestBase::SetUpOnMainThread() {
  // Set up the HTTPS server.
  https_server_ = std::make_unique<net::EmbeddedTestServer>(
      net::EmbeddedTestServer::TYPE_HTTPS);
  host_resolver()->AddRule("a.com", "127.0.0.1");
  host_resolver()->AddRule("b.com", "127.0.0.1");
  ASSERT_TRUE(https_server_->InitializeAndListen());
  https_server_->ServeFilesFromSourceDirectory("components/test/data/payments");
  https_server_->StartAcceptingConnections();

  url_fetcher_factory_ = std::make_unique<net::FakeURLFetcherFactory>(
      new net::URLFetcherImplFactory());

  ChromeAutofillClient* client =
      ChromeAutofillClient::FromWebContents(GetActiveWebContents());
  client->SetSaveCardBubbleControllerObserverForTest(this);

  NavigateTo(test_file_path_);
}

void SaveCardBubbleViewsBrowserTestBase::NavigateTo(
    const std::string& file_path) {
  if (file_path.find("data:") == 0U) {
    ui_test_utils::NavigateToURL(browser(), GURL(file_path));
  } else {
    ui_test_utils::NavigateToURL(browser(),
                                 https_server()->GetURL("a.com", file_path));
  }
}

void SaveCardBubbleViewsBrowserTestBase::OnOfferLocalSave() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::OFFERED_LOCAL_SAVE);
}

void SaveCardBubbleViewsBrowserTestBase::OnDecideToRequestUploadSave() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::REQUESTED_UPLOAD_SAVE);
}

void SaveCardBubbleViewsBrowserTestBase::OnDecideToNotRequestUploadSave() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::DID_NOT_REQUEST_UPLOAD_SAVE);
}

void SaveCardBubbleViewsBrowserTestBase::OnSentUploadCardRequest() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::SENT_UPLOAD_CARD_REQUEST);
}

void SaveCardBubbleViewsBrowserTestBase::OnLocalBubbleShown() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::BUBBLE_SHOWN_LOCAL);
}

void SaveCardBubbleViewsBrowserTestBase::OnUploadBubbleShown() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::BUBBLE_SHOWN_UPLOAD);
}

void SaveCardBubbleViewsBrowserTestBase::OnBubbleAccepted() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::BUBBLE_ACCEPTED);
}

void SaveCardBubbleViewsBrowserTestBase::OnBubbleCancelled() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::BUBBLE_CANCELLED);
}

void SaveCardBubbleViewsBrowserTestBase::OnBubbleClosed() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::BUBBLE_CLOSED);
}

void SaveCardBubbleViewsBrowserTestBase::OnBubbleHidden() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::BUBBLE_HIDDEN);
}

void SaveCardBubbleViewsBrowserTestBase::OnFooterShown() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::FOOTER_SHOWN);
}

void SaveCardBubbleViewsBrowserTestBase::OnFooterHidden() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::FOOTER_HIDDEN);
}

void SaveCardBubbleViewsBrowserTestBase::OnCvcRequestViewShown() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::CVC_REQUEST_VIEW_SHOWN);
}

void SaveCardBubbleViewsBrowserTestBase::OnCvcRequestViewAccepted() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::CVC_REQUEST_VIEW_ACCEPTED);
}

void SaveCardBubbleViewsBrowserTestBase::OnLearnMoreLinkClicked() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::LEARN_MORE_LINK_CLICKED);
}

void SaveCardBubbleViewsBrowserTestBase::OnLegalMessageLinkClicked() {
  if (event_observer_)
    event_observer_->Observe(DialogEvent::LEGAL_MESSAGE_LINK_CLICKED);
}

void SaveCardBubbleViewsBrowserTestBase::
    DisableAutofillUpstreamRequestCvcIfMissingExperiment() {
  scoped_feature_list_.InitAndDisableFeature(
      kAutofillUpstreamRequestCvcIfMissing);
}

void SaveCardBubbleViewsBrowserTestBase::
    DisableAutofillUpstreamShowNewUiExperiment() {
  scoped_feature_list_.InitAndDisableFeature(kAutofillUpstreamShowNewUi);
}

void SaveCardBubbleViewsBrowserTestBase::
    EnableAutofillUpstreamRequestCvcIfMissingExperiment() {
  scoped_feature_list_.InitAndEnableFeature(
      kAutofillUpstreamRequestCvcIfMissing);
}

void SaveCardBubbleViewsBrowserTestBase::
    EnableAutofillUpstreamShowNewUiExperiment() {
  scoped_feature_list_.InitAndEnableFeature(kAutofillUpstreamShowNewUi);
}

void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitForm() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithoutCvc() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_clear_cvc_button_js =
      "(function() { document.getElementById('clear_cvc').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_clear_cvc_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithInvalidCvc() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_invalid_cvc_button_js =
      "(function() { document.getElementById('invalid_cvc').click(); })();";
  ASSERT_TRUE(
      content::ExecuteScript(web_contents, click_invalid_cvc_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithAmexWithoutCvc() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_amex_button_js =
      "(function() { document.getElementById('fill_form_amex').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_amex_button_js));

  const std::string click_clear_cvc_button_js =
      "(function() { document.getElementById('clear_cvc').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_clear_cvc_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithoutName() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_clear_name_button_js =
      "(function() { document.getElementById('clear_name').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_clear_name_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::
    FillAndSubmitFormWithConflictingName() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_conflicting_name_button_js =
      "(function() { document.getElementById('conflicting_name').click(); "
      "})();";
  ASSERT_TRUE(
      content::ExecuteScript(web_contents, click_conflicting_name_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::FillAndSubmitFormWithoutAddress() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_clear_name_button_js =
      "(function() { document.getElementById('clear_address').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_clear_name_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::
    FillAndSubmitFormWithConflictingStreetAddress() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_conflicting_street_address_button_js =
      "(function() { "
      "document.getElementById('conflicting_street_address').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(
      web_contents, click_conflicting_street_address_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::
    FillAndSubmitFormWithConflictingPostalCode() {
  content::WebContents* web_contents = GetActiveWebContents();
  const std::string click_fill_button_js =
      "(function() { document.getElementById('fill_form').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_fill_button_js));

  const std::string click_conflicting_postal_code_button_js =
      "(function() { "
      "document.getElementById('conflicting_postal_code').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents,
                                     click_conflicting_postal_code_button_js));

  const std::string click_submit_button_js =
      "(function() { document.getElementById('submit').click(); })();";
  ASSERT_TRUE(content::ExecuteScript(web_contents, click_submit_button_js));
}

void SaveCardBubbleViewsBrowserTestBase::SetUploadDetailsRpcPaymentsAccepts() {
  url_fetcher_factory_->SetFakeResponse(
      GURL(kURLGetUploadDetailsRequest), kResponseGetUploadDetailsSuccess,
      net::HTTP_OK, net::URLRequestStatus::SUCCESS);
}

void SaveCardBubbleViewsBrowserTestBase::SetUploadDetailsRpcPaymentsDeclines() {
  url_fetcher_factory_->SetFakeResponse(
      GURL(kURLGetUploadDetailsRequest), kResponseGetUploadDetailsFailure,
      net::HTTP_OK, net::URLRequestStatus::SUCCESS);
}

void SaveCardBubbleViewsBrowserTestBase::SetUploadDetailsRpcServerError() {
  url_fetcher_factory_->SetFakeResponse(
      GURL(kURLGetUploadDetailsRequest), kResponseGetUploadDetailsSuccess,
      net::HTTP_INTERNAL_SERVER_ERROR, net::URLRequestStatus::FAILED);
}

void SaveCardBubbleViewsBrowserTestBase::ClickOnDialogViewWithIdAndWait(
    DialogViewId view_id) {
  views::View* specified_view = FindViewInBubbleById(view_id);
  DCHECK(specified_view);

  ui::MouseEvent pressed(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                         ui::EF_LEFT_MOUSE_BUTTON);
  specified_view->OnMousePressed(pressed);
  ui::MouseEvent released_event = ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, gfx::Point(), gfx::Point(), ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  specified_view->OnMouseReleased(released_event);

  WaitForObservedEvent();
}

views::View* SaveCardBubbleViewsBrowserTestBase::FindViewInBubbleById(
    DialogViewId view_id) {
  SaveCardBubbleViews* save_card_bubble_views =
      static_cast<SaveCardBubbleViews*>(
          SaveCardBubbleControllerImpl::FromWebContents(GetActiveWebContents())
              ->save_card_bubble_view());
  DCHECK(save_card_bubble_views);

  views::View* specified_view =
      save_card_bubble_views->GetViewByID(static_cast<int>(view_id));
  if (!specified_view) {
    // Many of the save card bubble's inner Views are not child views but rather
    // contained by its DialogClientView. If we didn't find what we were looking
    // for, check there as well.
    specified_view = save_card_bubble_views->GetDialogClientView()->GetViewByID(
        static_cast<int>(view_id));
  }
  if (!specified_view) {
    // Additionally, the save card bubble's footnote view is not part of its
    // main bubble, and contains elements such as the legal message links.
    // If we didn't find what we were looking for, check there as well.
    views::View* footnote_view = save_card_bubble_views->GetFootnoteView();
    if (footnote_view) {
      specified_view = footnote_view->GetViewByID(static_cast<int>(view_id));
    }
  }
  return specified_view;
}

content::WebContents*
SaveCardBubbleViewsBrowserTestBase::GetActiveWebContents() {
  return browser()->tab_strip_model()->GetActiveWebContents();
}

void SaveCardBubbleViewsBrowserTestBase::ResetEventObserverForSequence(
    std::list<DialogEvent> event_sequence) {
  event_observer_ = std::make_unique<DialogEventObserver<DialogEvent>>(
      std::move(event_sequence));
}

void SaveCardBubbleViewsBrowserTestBase::WaitForObservedEvent() {
  event_observer_->Wait();
}

}  // namespace autofill
