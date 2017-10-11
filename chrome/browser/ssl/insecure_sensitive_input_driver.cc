// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/insecure_sensitive_input_driver.h"

#include <utility>

#include "chrome/browser/ssl/insecure_sensitive_input_driver_factory.h"
#include "components/security_state/content/ssl_status_input_event_data.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

InsecureSensitiveInputDriver::InsecureSensitiveInputDriver(
    content::RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host) {}

InsecureSensitiveInputDriver::~InsecureSensitiveInputDriver() {}

// Creates or retrieves the |user_data| object in the SSLStatus attached to the
// WebContent's NavigationEntry.
security_state::SSLStatusInputEventData* GetOrCreateSSLStatusInputEventData(
    content::WebContents* web_contents) {
  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();
  // We aren't guaranteed to always have a navigation entry.
  if (!entry)
    return nullptr;

  content::SSLStatus& ssl = entry->GetSSL();
  security_state::SSLStatusInputEventData* input_events =
      static_cast<security_state::SSLStatusInputEventData*>(
          ssl.user_data.get());
  if (!input_events) {
    ssl.user_data = base::MakeUnique<security_state::SSLStatusInputEventData>();
    input_events = static_cast<security_state::SSLStatusInputEventData*>(
        ssl.user_data.get());
  }
  return input_events;
}

void InsecureSensitiveInputDriver::BindInsecureInputServiceRequest(
    blink::mojom::InsecureInputServiceRequest request) {
  insecure_input_bindings_.AddBinding(this, std::move(request));
}

void InsecureSensitiveInputDriver::DidEditCreditCardFieldInInsecureContext() {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host_);

  security_state::SSLStatusInputEventData* input_events =
      GetOrCreateSSLStatusInputEventData(web_contents);
  if (!input_events)
    return;

  // If the first credit card field edit for the web contents was just
  // performed, update the SSLStatusInputEventData.
  bool old_value = input_events->input_events()->credit_card_field_edited;
  if (!old_value) {
    input_events->input_events()->credit_card_field_edited = true;
    web_contents->DidChangeVisibleSecurityState();
  }
}

void InsecureSensitiveInputDriver::PasswordFieldVisibleInInsecureContext() {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host_);
  InsecureSensitiveInputDriverFactory* parent =
      InsecureSensitiveInputDriverFactory::GetOrCreateForWebContents(
          web_contents);

  parent->RenderFrameHasVisiblePasswordField(render_frame_host_);

  security_state::SSLStatusInputEventData* input_events =
      GetOrCreateSSLStatusInputEventData(web_contents);
  if (!input_events)
    return;

  // If the first password field was just added to the web contents,
  // update the SSLStatusInputEventData.
  bool old_value = input_events->input_events()->password_field_shown;
  if (!old_value) {
    input_events->input_events()->password_field_shown = true;
    web_contents->DidChangeVisibleSecurityState();
  }
}

void InsecureSensitiveInputDriver::
    AllPasswordFieldsInInsecureContextInvisible() {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host_);
  InsecureSensitiveInputDriverFactory* parent =
      InsecureSensitiveInputDriverFactory::GetOrCreateForWebContents(
          web_contents);

  parent->RenderFrameHasNoVisiblePasswordFields(render_frame_host_);

  security_state::SSLStatusInputEventData* input_events =
      GetOrCreateSSLStatusInputEventData(web_contents);
  if (!input_events)
    return;

  // If the last password field was just removed from the web contents,
  // update the SSLStatusInputEventData.
  bool old_value = input_events->input_events()->password_field_shown;
  if (old_value && !parent->HasVisiblePasswordFields()) {
    input_events->input_events()->password_field_shown = false;
    web_contents->DidChangeVisibleSecurityState();
  }
}

void InsecureSensitiveInputDriver::DidEditFieldInInsecureContext() {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host_);
  security_state::SSLStatusInputEventData* input_events =
      GetOrCreateSSLStatusInputEventData(web_contents);
  if (!input_events)
    return;

  // If the first field edit in the web contents was just performed,
  // update the SSLStatusInputEventData.
  bool old_value = input_events->input_events()->insecure_field_edited;
  if (!old_value) {
    input_events->input_events()->insecure_field_edited = true;
    web_contents->DidChangeVisibleSecurityState();
  }
}
