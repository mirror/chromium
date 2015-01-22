// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webcontentdecryptionmodulesession_impl.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "media/base/cdm_key_information.h"
#include "media/base/cdm_promise.h"
#include "media/base/media_keys.h"
#include "media/blink/cdm_result_promise.h"
#include "media/blink/cdm_session_adapter.h"
#include "media/blink/new_session_cdm_result_promise.h"
#include "third_party/WebKit/public/platform/WebData.h"
#include "third_party/WebKit/public/platform/WebEncryptedMediaKeyInformation.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebVector.h"

namespace media {

const char kCreateSessionUMAName[] = "CreateSession";
const char kLoadSessionUMAName[] = "LoadSession";

// TODO(jrummell): Pass an enum from blink. http://crbug.com/418239.
const char kTemporarySessionType[] = "temporary";
const char kPersistentLicenseSessionType[] = "persistent-license";
const char kPersistentReleaseMessageSessionType[] =
    "persistent-release-message";

static blink::WebContentDecryptionModuleSession::Client::MessageType
convertMessageType(MediaKeys::MessageType message_type) {
  switch (message_type) {
    case media::MediaKeys::LICENSE_REQUEST:
      return blink::WebContentDecryptionModuleSession::Client::MessageType::
          LicenseRequest;
    case media::MediaKeys::LICENSE_RENEWAL:
      return blink::WebContentDecryptionModuleSession::Client::MessageType::
          LicenseRenewal;
    case media::MediaKeys::LICENSE_RELEASE:
      return blink::WebContentDecryptionModuleSession::Client::MessageType::
          LicenseRelease;
  }

  NOTREACHED();
  return blink::WebContentDecryptionModuleSession::Client::MessageType::
      LicenseRequest;
}

static blink::WebEncryptedMediaKeyInformation::KeyStatus convertStatus(
    media::CdmKeyInformation::KeyStatus status) {
  switch (status) {
    case media::CdmKeyInformation::USABLE:
      return blink::WebEncryptedMediaKeyInformation::KeyStatus::Usable;
    case media::CdmKeyInformation::INTERNAL_ERROR:
      return blink::WebEncryptedMediaKeyInformation::KeyStatus::InternalError;
    case media::CdmKeyInformation::EXPIRED:
      return blink::WebEncryptedMediaKeyInformation::KeyStatus::Expired;
    case media::CdmKeyInformation::OUTPUT_NOT_ALLOWED:
      return blink::WebEncryptedMediaKeyInformation::KeyStatus::
          OutputNotAllowed;
  }

  NOTREACHED();
  return blink::WebEncryptedMediaKeyInformation::KeyStatus::InternalError;
}

WebContentDecryptionModuleSessionImpl::WebContentDecryptionModuleSessionImpl(
    const scoped_refptr<CdmSessionAdapter>& adapter)
    : adapter_(adapter), is_closed_(false), weak_ptr_factory_(this) {
}

WebContentDecryptionModuleSessionImpl::
    ~WebContentDecryptionModuleSessionImpl() {
  if (!web_session_id_.empty())
    adapter_->UnregisterSession(web_session_id_);
}

void WebContentDecryptionModuleSessionImpl::setClientInterface(Client* client) {
  client_ = client;
}

blink::WebString WebContentDecryptionModuleSessionImpl::sessionId() const {
  return blink::WebString::fromUTF8(web_session_id_);
}

void WebContentDecryptionModuleSessionImpl::initializeNewSession(
    const blink::WebString& init_data_type,
    const uint8* init_data,
    size_t init_data_length) {
  // TODO(jrummell): Remove once blink updated.
  NOTREACHED();
}

void WebContentDecryptionModuleSessionImpl::update(const uint8* response,
                                                   size_t response_length) {
  // TODO(jrummell): Remove once blink updated.
  NOTREACHED();
}

void WebContentDecryptionModuleSessionImpl::release() {
  // TODO(jrummell): Remove once blink updated.
  NOTREACHED();
}

void WebContentDecryptionModuleSessionImpl::initializeNewSession(
    const blink::WebString& init_data_type,
    const uint8* init_data,
    size_t init_data_length,
    const blink::WebString& session_type,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(web_session_id_.empty());

  // TODO(ddorwin): Guard against this in supported types check and remove this.
  // Chromium only supports ASCII MIME types.
  if (!base::IsStringASCII(init_data_type)) {
    NOTREACHED();
    std::string message = "The initialization data type " +
                          init_data_type.utf8() +
                          " is not supported by the key system.";
    result.completeWithError(
        blink::WebContentDecryptionModuleExceptionNotSupportedError, 0,
        blink::WebString::fromUTF8(message));
    return;
  }

  std::string init_data_type_as_ascii = base::UTF16ToASCII(init_data_type);
  DLOG_IF(WARNING, init_data_type_as_ascii.find('/') != std::string::npos)
      << "init_data_type '" << init_data_type_as_ascii
      << "' may be a MIME type";

  MediaKeys::SessionType session_type_enum;
  if (session_type == kPersistentLicenseSessionType) {
    session_type_enum = MediaKeys::PERSISTENT_LICENSE_SESSION;
  } else if (session_type == kPersistentReleaseMessageSessionType) {
    session_type_enum = MediaKeys::PERSISTENT_RELEASE_MESSAGE_SESSION;
  } else {
    DCHECK(session_type == kTemporarySessionType);
    session_type_enum = MediaKeys::TEMPORARY_SESSION;
  }

  adapter_->InitializeNewSession(
      init_data_type_as_ascii, init_data,
      base::saturated_cast<int>(init_data_length), session_type_enum,
      scoped_ptr<NewSessionCdmPromise>(new NewSessionCdmResultPromise(
          result, adapter_->GetKeySystemUMAPrefix() + kCreateSessionUMAName,
          base::Bind(
              &WebContentDecryptionModuleSessionImpl::OnSessionInitialized,
              base::Unretained(this)))));
}

void WebContentDecryptionModuleSessionImpl::load(
    const blink::WebString& session_id,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(!session_id.isEmpty());
  DCHECK(web_session_id_.empty());

  // TODO(jrummell): Now that there are 2 types of persistent sessions, the
  // session type should be passed from blink. Type should also be passed in the
  // constructor (and removed from initializeNewSession()).
  adapter_->LoadSession(
      MediaKeys::PERSISTENT_LICENSE_SESSION, base::UTF16ToASCII(session_id),
      scoped_ptr<NewSessionCdmPromise>(new NewSessionCdmResultPromise(
          result, adapter_->GetKeySystemUMAPrefix() + kLoadSessionUMAName,
          base::Bind(
              &WebContentDecryptionModuleSessionImpl::OnSessionInitialized,
              base::Unretained(this)))));
}

void WebContentDecryptionModuleSessionImpl::update(
    const uint8* response,
    size_t response_length,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(response);
  DCHECK(!web_session_id_.empty());
  adapter_->UpdateSession(web_session_id_, response,
                          base::saturated_cast<int>(response_length),
                          scoped_ptr<SimpleCdmPromise>(
                              new CdmResultPromise<>(result, std::string())));
}

void WebContentDecryptionModuleSessionImpl::close(
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(!web_session_id_.empty());
  adapter_->CloseSession(web_session_id_,
                         scoped_ptr<SimpleCdmPromise>(
                             new CdmResultPromise<>(result, std::string())));
}

void WebContentDecryptionModuleSessionImpl::remove(
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(!web_session_id_.empty());
  adapter_->RemoveSession(web_session_id_,
                          scoped_ptr<SimpleCdmPromise>(
                              new CdmResultPromise<>(result, std::string())));
}

void WebContentDecryptionModuleSessionImpl::release(
    blink::WebContentDecryptionModuleResult result) {
  close(result);
}

void WebContentDecryptionModuleSessionImpl::OnSessionMessage(
    MediaKeys::MessageType message_type,
    const std::vector<uint8>& message) {
  DCHECK(client_) << "Client not set before message event";
  client_->message(convertMessageType(message_type),
                   message.empty() ? NULL : &message[0], message.size());
}

void WebContentDecryptionModuleSessionImpl::OnSessionKeysChange(
    bool has_additional_usable_key,
    CdmKeysInfo keys_info) {
  blink::WebVector<blink::WebEncryptedMediaKeyInformation> keys(
      keys_info.size());
  for (size_t i = 0; i < keys_info.size(); ++i) {
    const auto& key_info = keys_info[i];
    keys[i].setId(blink::WebData(reinterpret_cast<char*>(&key_info->key_id[0]),
                                 key_info->key_id.size()));
    keys[i].setStatus(convertStatus(key_info->status));
    keys[i].setSystemCode(key_info->system_code);
  }

  // Now send the event to blink.
  client_->keysStatusesChange(keys, has_additional_usable_key);
}

void WebContentDecryptionModuleSessionImpl::OnSessionExpirationUpdate(
    const base::Time& new_expiry_time) {
  client_->expirationChanged(new_expiry_time.ToJsTime());
}

void WebContentDecryptionModuleSessionImpl::OnSessionClosed() {
  if (is_closed_)
    return;

  is_closed_ = true;
  client_->close();
}

blink::WebContentDecryptionModuleResult::SessionStatus
WebContentDecryptionModuleSessionImpl::OnSessionInitialized(
    const std::string& web_session_id) {
  // CDM will return NULL if the session to be loaded can't be found.
  if (web_session_id.empty())
    return blink::WebContentDecryptionModuleResult::SessionNotFound;

  DCHECK(web_session_id_.empty()) << "Session ID may not be changed once set.";
  web_session_id_ = web_session_id;
  return adapter_->RegisterSession(web_session_id_,
                                   weak_ptr_factory_.GetWeakPtr())
             ? blink::WebContentDecryptionModuleResult::NewSession
             : blink::WebContentDecryptionModuleResult::SessionAlreadyExists;
}

}  // namespace media
