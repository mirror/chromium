// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/autofill/web_view_autofill_client.h"

#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/signin/core/browser/profile_identity_provider.h"
#include "components/signin/core/browser/signin_manager.h"
#import "ios/web/public/web_state/web_state.h"
#include "ios/web_view/internal/autofill/web_view_personal_data_manager_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/internal/webdata_services/web_view_web_data_service_wrapper_factory.h"

namespace ios_web_view {

WebViewAutofillClient::WebViewAutofillClient(web::WebState* web_state,
                                             id<AutofillClientIOSBridge> bridge)
    : web_state_(web_state), bridge_(bridge) {
  browser_state_ =
      WebViewBrowserState::FromBrowserState(web_state_->GetBrowserState());

  identity_provider_.reset(new ProfileIdentityProvider(
      WebViewSigninManagerFactory::GetForBrowserState(browser_state_),
      WebViewOAuth2TokenServiceFactory::GetForBrowserState(browser_state_),
      base::Closure()));
}

WebViewAutofillClient::~WebViewAutofillClient() {
  HideAutofillPopup();
}

autofill::PersonalDataManager* WebViewAutofillClient::GetPersonalDataManager() {
  return WebViewPersonalDataManagerFactory::GetForBrowserState(browser_state_);
}

PrefService* WebViewAutofillClient::GetPrefs() {
  return browser_state_->GetPrefs();
}

// TODO(crbug.com/535784): Implement this when adding credit card upload.
syncer::SyncService* WebViewAutofillClient::GetSyncService() {
  return nullptr;
}

IdentityProvider* WebViewAutofillClient::GetIdentityProvider() {
  return identity_provider_.get();
}

ukm::UkmRecorder* WebViewAutofillClient::GetUkmRecorder() {
  return nullptr;
}

autofill::SaveCardBubbleController*
WebViewAutofillClient::GetSaveCardBubbleController() {
  return nullptr;
}

void WebViewAutofillClient::ShowAutofillSettings() {
  NOTREACHED();
}

void WebViewAutofillClient::ShowUnmaskPrompt(
    const autofill::CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<autofill::CardUnmaskDelegate> delegate) {}

void WebViewAutofillClient::OnUnmaskVerificationResult(
    PaymentsRpcResult result) {}

void WebViewAutofillClient::ConfirmSaveCreditCardLocally(
    const autofill::CreditCard& card,
    const base::Closure& callback) {}

void WebViewAutofillClient::ConfirmSaveCreditCardToCloud(
    const autofill::CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    bool should_cvc_be_requested,
    const base::Closure& callback) {}

void WebViewAutofillClient::ConfirmCreditCardFillAssist(
    const autofill::CreditCard& card,
    const base::Closure& callback) {}

void WebViewAutofillClient::LoadRiskData(
    const base::Callback<void(const std::string&)>& callback) {}

bool WebViewAutofillClient::HasCreditCardScanFeature() {
  return false;
}

void WebViewAutofillClient::ScanCreditCard(
    const CreditCardScanCallback& callback) {
  NOTREACHED();
}

void WebViewAutofillClient::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<autofill::Suggestion>& suggestions,
    base::WeakPtr<autofill::AutofillPopupDelegate> delegate) {
  [bridge_ showAutofillPopup:suggestions popupDelegate:delegate];
}

void WebViewAutofillClient::HideAutofillPopup() {
  [bridge_ hideAutofillPopup];
}

bool WebViewAutofillClient::IsAutocompleteEnabled() {
  // For browser, Autocomplete is always enabled as part of Autofill.
  return browser_state_->GetPrefs()->GetBoolean(
      autofill::prefs::kAutofillEnabled);
}

void WebViewAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  NOTREACHED();
}

void WebViewAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) {}

void WebViewAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {}

scoped_refptr<autofill::AutofillWebDataService>
WebViewAutofillClient::GetDatabase() {
  return WebViewWebDataServiceWrapperFactory::GetAutofillWebDataForBrowserState(
      browser_state_, ServiceAccessType::EXPLICIT_ACCESS);
}

bool WebViewAutofillClient::IsContextSecure() {
  return false;
}

bool WebViewAutofillClient::ShouldShowSigninPromo() {
  return false;
}

void WebViewAutofillClient::ExecuteCommand(int id) {
  NOTIMPLEMENTED();
}

bool WebViewAutofillClient::IsAutofillSupported() {
  return true;
}

}  // namespace ios_web_view
