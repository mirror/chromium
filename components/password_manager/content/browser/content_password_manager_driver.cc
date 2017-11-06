// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/browser/content_password_manager_driver.h"

#include <utility>

#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/common/url_utils.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/content/browser/bad_message.h"
#include "components/password_manager/content/browser/content_password_manager_driver_factory.h"
#include "components/password_manager/core/browser/log_manager.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_recorder.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "net/cert/cert_status_flags.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace password_manager {

ContentPasswordManagerDriver::ContentPasswordManagerDriver(
    content::RenderFrameHost* render_frame_host,
    PasswordManagerClient* client,
    autofill::AutofillClient* autofill_client)
    : render_frame_host_(render_frame_host),
      client_(client),
      password_generation_manager_(client, this),
      password_autofill_manager_(this, autofill_client, client),
      next_free_key_(0),
      is_main_frame_(render_frame_host->GetParent() == nullptr),
      password_manager_binding_(this),
      weak_factory_(this) {
  // For some frames |this| may be instantiated before log manager creation, so
  // here we can not send logging state to renderer process for them. For such
  // cases, after the log manager got ready later,
  // ContentPasswordManagerDriverFactory::RequestSendLoggingAvailability() will
  // call ContentPasswordManagerDriver::SendLoggingAvailability() on |this| to
  // do it actually.
  if (client_->GetLogManager()) {
    // Do not call the virtual method SendLoggingAvailability from a constructor
    // here, inline its steps instead.
    GetPasswordAutofillAgent()->SetLoggingState(
        client_->GetLogManager()->IsLoggingActive());
  }
}

ContentPasswordManagerDriver::~ContentPasswordManagerDriver() {
}

// static
ContentPasswordManagerDriver*
ContentPasswordManagerDriver::GetForRenderFrameHost(
    content::RenderFrameHost* render_frame_host) {
  ContentPasswordManagerDriverFactory* factory =
      ContentPasswordManagerDriverFactory::FromWebContents(
          content::WebContents::FromRenderFrameHost(render_frame_host));
  return factory ? factory->GetDriverForFrame(render_frame_host) : nullptr;
}

void ContentPasswordManagerDriver::BindRequest(
    autofill::mojom::PasswordManagerDriverRequest request) {
  password_manager_binding_.Bind(std::move(request));
}

void ContentPasswordManagerDriver::FillPasswordForm(
    const autofill::PasswordFormFillData& form_data) {
  const int key = next_free_key_++;
  password_autofill_manager_.OnAddPasswordFormMapping(key, form_data);
  GetPasswordAutofillAgent()->FillPasswordForm(
      key, autofill::ClearPasswordValues(form_data));
}

void ContentPasswordManagerDriver::AllowPasswordGenerationForForm(
    const autofill::PasswordForm& form) {
  if (!GetPasswordGenerationManager()->IsGenerationEnabled())
    return;
  GetPasswordGenerationAgent()->FormNotBlacklisted(form);
}

void ContentPasswordManagerDriver::FormsEligibleForGenerationFound(
    const std::vector<autofill::PasswordFormGenerationData>& forms) {
  GetPasswordGenerationAgent()->FoundFormsEligibleForGeneration(forms);
}

void ContentPasswordManagerDriver::AutofillDataReceived(
    const std::map<autofill::FormData,
                   autofill::PasswordFormFieldPredictionMap>& predictions) {
  GetPasswordAutofillAgent()->AutofillUsernameAndPasswordDataReceived(
      predictions);
}

void ContentPasswordManagerDriver::GeneratedPasswordAccepted(
    const base::string16& password) {
  GetPasswordGenerationAgent()->GeneratedPasswordAccepted(password);
}

void ContentPasswordManagerDriver::UserSelectedManualGenerationOption() {
  GetPasswordGenerationAgent()->UserSelectedManualGenerationOption();
}

void ContentPasswordManagerDriver::FillSuggestion(
    const base::string16& username,
    const base::string16& password) {
  GetAutofillAgent()->FillPasswordSuggestion(username, password);
}

void ContentPasswordManagerDriver::PreviewSuggestion(
    const base::string16& username,
    const base::string16& password) {
  GetAutofillAgent()->PreviewPasswordSuggestion(username, password);
}

void ContentPasswordManagerDriver::ShowInitialPasswordAccountSuggestions(
    const autofill::PasswordFormFillData& form_data) {
  const int key = next_free_key_++;
  password_autofill_manager_.OnAddPasswordFormMapping(key, form_data);
  GetAutofillAgent()->ShowInitialPasswordAccountSuggestions(key, form_data);
}

void ContentPasswordManagerDriver::ClearPreviewedForm() {
  GetAutofillAgent()->ClearPreviewedForm();
}

void ContentPasswordManagerDriver::ForceSavePassword() {
  GetPasswordAutofillAgent()->FindFocusedPasswordForm(
      base::Bind(&ContentPasswordManagerDriver::OnFocusedPasswordFormFound,
                 weak_factory_.GetWeakPtr()));
}

void ContentPasswordManagerDriver::ShowManualFallbackForSaving(
    const autofill::PasswordForm& untrusted_password_form) {
  autofill::PasswordForm password_form(untrusted_password_form);
  password_form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->ShowManualFallbackForSaving(this, password_form);
}

void ContentPasswordManagerDriver::HideManualFallbackForSaving() {
  GetPasswordManager()->HideManualFallbackForSaving();
}

void ContentPasswordManagerDriver::GeneratePassword() {
  GetPasswordGenerationAgent()->UserTriggeredGeneratePassword();
}

void ContentPasswordManagerDriver::SendLoggingAvailability() {
  GetPasswordAutofillAgent()->SetLoggingState(
      client_->GetLogManager()->IsLoggingActive());
}

void ContentPasswordManagerDriver::AllowToRunFormClassifier() {
  GetPasswordGenerationAgent()->AllowToRunFormClassifier();
}

autofill::AutofillDriver* ContentPasswordManagerDriver::GetAutofillDriver() {
  return autofill::ContentAutofillDriver::GetForRenderFrameHost(
      render_frame_host_);
}

bool ContentPasswordManagerDriver::IsMainFrame() const {
  return is_main_frame_;
}

void ContentPasswordManagerDriver::MatchingBlacklistedFormFound() {
  GetPasswordAutofillAgent()->BlacklistedFormFound();
}

PasswordGenerationManager*
ContentPasswordManagerDriver::GetPasswordGenerationManager() {
  return &password_generation_manager_;
}

PasswordManager* ContentPasswordManagerDriver::GetPasswordManager() {
  return client_->GetPasswordManager();
}

PasswordAutofillManager*
ContentPasswordManagerDriver::GetPasswordAutofillManager() {
  return &password_autofill_manager_;
}

void ContentPasswordManagerDriver::PasswordFormsParsed(
    const std::vector<autofill::PasswordForm>& untrusted_forms) {
  std::vector<autofill::PasswordForm> forms(untrusted_forms);
  for (auto& form : forms)
    form.origin = GetCanonicalOriginForDocument();

  OnPasswordFormsParsedNoRenderCheck(forms);
}

void ContentPasswordManagerDriver::OnPasswordFormsParsedNoRenderCheck(
    const std::vector<autofill::PasswordForm>& forms) {
  GetPasswordManager()->OnPasswordFormsParsed(this, forms);
  GetPasswordGenerationManager()->CheckIfFormClassifierShouldRun();
}

void ContentPasswordManagerDriver::PasswordFormsRendered(
    const std::vector<autofill::PasswordForm>& untrusted_visible_forms,
    bool did_stop_loading) {
  std::vector<autofill::PasswordForm> visible_forms(untrusted_visible_forms);
  for (auto& form : visible_forms)
    form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->OnPasswordFormsRendered(this, visible_forms,
                                                did_stop_loading);
}

void ContentPasswordManagerDriver::PasswordFormSubmitted(
    const autofill::PasswordForm& untrusted_password_form) {
  autofill::PasswordForm password_form(untrusted_password_form);
  password_form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->OnPasswordFormSubmitted(this, password_form);
}

void ContentPasswordManagerDriver::OnFocusedPasswordFormFound(
    const autofill::PasswordForm& untrusted_password_form) {
  autofill::PasswordForm password_form(untrusted_password_form);
  password_form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->OnPasswordFormForceSaveRequested(this, password_form);
}

void ContentPasswordManagerDriver::DidNavigateFrame(
    content::NavigationHandle* navigation_handle) {
  // Clear page specific data after main frame navigation.
  if (navigation_handle->IsInMainFrame() &&
      !navigation_handle->IsSameDocument()) {
    GetPasswordManager()->DidNavigateMainFrame();
    GetPasswordAutofillManager()->DidNavigateMainFrame();
  }
}

void ContentPasswordManagerDriver::InPageNavigation(
    const autofill::PasswordForm& untrusted_password_form) {
  autofill::PasswordForm password_form(untrusted_password_form);
  password_form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->OnInPageNavigation(this, password_form);
}

void ContentPasswordManagerDriver::PresaveGeneratedPassword(
    const autofill::PasswordForm& untrusted_password_form) {
  autofill::PasswordForm password_form(untrusted_password_form);
  password_form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->OnPresaveGeneratedPassword(password_form);
}

void ContentPasswordManagerDriver::PasswordNoLongerGenerated(
    const autofill::PasswordForm& untrusted_password_form) {
  autofill::PasswordForm password_form(untrusted_password_form);
  password_form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->OnPasswordNoLongerGenerated(password_form);
}

void ContentPasswordManagerDriver::SaveGenerationFieldDetectedByClassifier(
    const autofill::PasswordForm& untrusted_password_form,
    const base::string16& generation_field) {
  autofill::PasswordForm password_form(untrusted_password_form);
  password_form.origin = GetCanonicalOriginForDocument();

  GetPasswordManager()->SaveGenerationFieldDetectedByClassifier(
      password_form, generation_field);
}

void ContentPasswordManagerDriver::CheckSafeBrowsingReputation(
    const GURL& form_action,
    const GURL& frame_url) {
#if defined(SAFE_BROWSING_DB_LOCAL)
  client_->CheckSafeBrowsingReputation(form_action, frame_url);
#endif
}

void ContentPasswordManagerDriver::ShowPasswordSuggestions(
    int key,
    base::i18n::TextDirection text_direction,
    const base::string16& typed_username,
    int options,
    const gfx::RectF& bounds) {
  password_autofill_manager_.OnShowPasswordSuggestions(
      key, text_direction, typed_username, options,
      TransformToRootCoordinates(bounds));
}

void ContentPasswordManagerDriver::ShowNotSecureWarning(
    base::i18n::TextDirection text_direction,
    const gfx::RectF& bounds) {
  password_autofill_manager_.OnShowNotSecureWarning(
      text_direction, TransformToRootCoordinates(bounds));
}

void ContentPasswordManagerDriver::ShowManualFallbackSuggestion(
    base::i18n::TextDirection text_direction,
    const gfx::RectF& bounds) {
  password_autofill_manager_.OnShowManualFallbackSuggestion(
      text_direction, TransformToRootCoordinates(bounds));
}

void ContentPasswordManagerDriver::RecordSavePasswordProgress(
    const std::string& log) {
  client_->GetLogManager()->LogSavePasswordProgress(log);
}

void ContentPasswordManagerDriver::UserModifiedPasswordField() {
  client_->GetMetricsRecorder().RecordUserModifiedPasswordField();
}

GURL ContentPasswordManagerDriver::GetCanonicalOriginForDocument() {
  GURL document_url = render_frame_host_->GetLastCommittedURL();
  return autofill::StripAuthAndParams(document_url);
}

bool ContentPasswordManagerDriver::CheckChildProcessSecurityPolicy(
    const GURL& url,
    BadMessageReason reason) {
  // Renderer-side logic should prevent any password manager usage for
  // about:blank frames as well as data URLs.  If that's not the case, kill the
  // renderer, as it might be exploited.
  if (url.SchemeIs(url::kAboutScheme) || url.SchemeIs(url::kDataScheme)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(), reason);
    return false;
  }

  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  if (!policy->CanAccessDataForOrigin(render_frame_host_->GetProcess()->GetID(),
                                      url)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(), reason);
    return false;
  }

  return true;
}

const autofill::mojom::AutofillAgentPtr&
ContentPasswordManagerDriver::GetAutofillAgent() {
  autofill::ContentAutofillDriver* autofill_driver =
      autofill::ContentAutofillDriver::GetForRenderFrameHost(
          render_frame_host_);
  DCHECK(autofill_driver);
  return autofill_driver->GetAutofillAgent();
}

const autofill::mojom::PasswordAutofillAgentPtr&
ContentPasswordManagerDriver::GetPasswordAutofillAgent() {
  if (!password_autofill_agent_) {
    auto request = mojo::MakeRequest(&password_autofill_agent_);
    // Some test environments may have no remote interface support.
    if (render_frame_host_->GetRemoteInterfaces()) {
      render_frame_host_->GetRemoteInterfaces()->GetInterface(
          std::move(request));
    }
  }

  return password_autofill_agent_;
}

const autofill::mojom::PasswordGenerationAgentPtr&
ContentPasswordManagerDriver::GetPasswordGenerationAgent() {
  if (!password_gen_agent_) {
    render_frame_host_->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&password_gen_agent_));
  }

  return password_gen_agent_;
}

gfx::RectF ContentPasswordManagerDriver::TransformToRootCoordinates(
    const gfx::RectF& bounds_in_frame_coordinates) {
  content::RenderWidgetHostView* rwhv = render_frame_host_->GetView();
  if (!rwhv)
    return bounds_in_frame_coordinates;
  return gfx::RectF(rwhv->TransformPointToRootCoordSpaceF(
                        bounds_in_frame_coordinates.origin()),
                    bounds_in_frame_coordinates.size());
}

}  // namespace password_manager
