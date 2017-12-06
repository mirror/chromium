// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEWS_BROWSERTEST_BASE_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEWS_BROWSERTEST_BASE_H_

#include <list>
#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/views/autofill/dialog_event_waiter.h"
#include "chrome/browser/ui/views/autofill/dialog_view_ids.h"
#include "chrome/browser/ui/views/autofill/save_card_bubble_views.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/core/browser/credit_card_save_manager.h"
#include "content/public/browser/web_contents_observer.h"
#include "device/geolocation/public/interfaces/geolocation_context.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace autofill {

// Base class for any interactive SaveCardBubbleViews browser test that will
// need to show and interact with the offer-to-save bubble.
class SaveCardBubbleViewsBrowserTestBase
    : public InProcessBrowserTest,
      public CreditCardSaveManager::ObserverForTest,
      public SaveCardBubbleController::ObserverForTest {
 public:
  // Various events that can be waited on by the DialogEventWaiter.
  enum DialogEvent : int {
    OFFERED_LOCAL_SAVE,
    REQUESTED_UPLOAD_SAVE,
    DID_NOT_REQUEST_UPLOAD_SAVE,
    SENT_UPLOAD_CARD_REQUEST,
    BUBBLE_SHOWN_LOCAL,
    BUBBLE_SHOWN_UPLOAD,
    BUBBLE_ACCEPTED,
    BUBBLE_CANCELLED,
    BUBBLE_CLOSED,
    BUBBLE_HIDDEN,
    FOOTER_SHOWN,
    FOOTER_HIDDEN,
    CVC_REQUEST_VIEW_SHOWN,
    CVC_REQUEST_VIEW_ACCEPTED,
    LEARN_MORE_LINK_CLICKED,
    LEGAL_MESSAGE_LINK_CLICKED,
  };

 protected:
  // Test will open a browser window to |test_file_path| (relative to
  // components/test/data/autofill).
  explicit SaveCardBubbleViewsBrowserTestBase(
      const std::string& test_file_path);
  ~SaveCardBubbleViewsBrowserTestBase() override;

  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpOnMainThread() override;

  void NavigateTo(const std::string& file_path);

  // CreditCardSaveManager::ObserverForTest:
  void OnOfferLocalSave() override;
  void OnDecideToRequestUploadSave() override;
  void OnDecideToNotRequestUploadSave() override;
  void OnSentUploadCardRequest() override;

  // SaveCardBubbleController::ObserverForTest:
  void OnLocalBubbleShown() override;
  void OnUploadBubbleShown() override;
  void OnBubbleAccepted() override;
  void OnBubbleCancelled() override;
  void OnBubbleClosed() override;
  void OnBubbleHidden() override;
  void OnCvcRequestViewShown() override;
  void OnCvcRequestViewAccepted() override;
  void OnLearnMoreLinkClicked() override;
  void OnLegalMessageLinkClicked() override;

  // Experiments.
  void DisableAutofillUpstreamRequestCvcIfMissingExperiment();
  void DisableAutofillUpstreamShowNewUiExperiment();
  void DisableSecondaryUiMdExperiment();
  void EnableAutofillUpstreamRequestCvcIfMissingExperiment();
  void EnableAutofillUpstreamShowNewUiExperiment();
  void EnableSecondaryUiMdExperiment();

  // Will call JavaScript to fill and submit the form in different ways.
  void FillAndSubmitForm();
  void FillAndSubmitFormWithoutCvc();
  void FillAndSubmitFormWithInvalidCvc();
  void FillAndSubmitFormWithAmexWithoutCvc();
  void FillAndSubmitFormWithoutName();
  void FillAndSubmitFormWithConflictingName();
  void FillAndSubmitFormWithoutAddress();
  void FillAndSubmitFormWithConflictingStreetAddress();
  void FillAndSubmitFormWithConflictingPostalCode();

  // For setting up Payments RPCs.
  void SetUploadDetailsRpcPaymentsAccepts();
  void SetUploadDetailsRpcPaymentsDeclines();
  void SetUploadDetailsRpcServerError();

  // Clicks on a view from within the dialog and waits for a certain event to be
  // observed.
  void ClickOnDialogViewWithIdAndWait(DialogViewId view_id);
  views::View* FindViewInBubbleById(DialogViewId view_id);

  // Gets the views::View* instance of the save card bubble.
  SaveCardBubbleViews* GetSaveCardBubbleViews();

  content::WebContents* GetActiveWebContents();

  // Resets the event waiter for a given |event_sequence|.
  void ResetEventWaiterForSequence(std::list<DialogEvent> event_sequence);
  // Wait for the event(s) passed to ResetEventWaiter*() to occur.
  void WaitForObservedEvent();

 private:
  std::unique_ptr<DialogEventWaiter<DialogEvent>> event_waiter_;
  std::unique_ptr<net::FakeURLFetcherFactory> url_fetcher_factory_;
  base::test::ScopedFeatureList scoped_feature_list_;
  const std::string test_file_path_;

  // FakeGeolocation setup:
  class FakeGeolocation;
  std::unique_ptr<FakeGeolocation> fake_geolocation_;
  std::unique_ptr<service_manager::Connector> connector_;
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(SaveCardBubbleViewsBrowserTestBase);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_SAVE_CARD_BUBBLE_VIEWS_BROWSERTEST_BASE_H_
