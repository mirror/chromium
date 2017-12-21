// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/cryptohome/cryptohome_util.h"

#include <string>

#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome/key.pb.h"
#include "chromeos/dbus/cryptohome/rpc.pb.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "third_party/protobuf/src/google/protobuf/repeated_field.h"

using google::protobuf::RepeatedPtrField;

namespace cryptohome {

AuthorizationRequest CreateAuthorizationRequest(const std::string& label,
                                                const std::string& secret) {
  cryptohome::AuthorizationRequest auth_request;
  Key* key = auth_request.mutable_key();
  if (!label.empty())
    key->mutable_data()->set_label(label);

  key->set_secret(secret);
  return auth_request;
}

void KeyDefinitionToKey(const KeyDefinition& key_def, Key* key) {
  key->set_secret(key_def.secret);
  KeyData* data = key->mutable_data();
  DCHECK_EQ(KeyDefinition::TYPE_PASSWORD, key_def.type);
  data->set_type(KeyData::KEY_TYPE_PASSWORD);
  data->set_label(key_def.label);

  if (key_def.revision > 0)
    data->set_revision(key_def.revision);

  if (key_def.privileges != 0) {
    KeyPrivileges* privileges = data->mutable_privileges();
    privileges->set_mount(key_def.privileges & PRIV_MOUNT);
    privileges->set_add(key_def.privileges & PRIV_ADD);
    privileges->set_remove(key_def.privileges & PRIV_REMOVE);
    privileges->set_update(key_def.privileges & PRIV_MIGRATE);
    privileges->set_authorized_update(key_def.privileges &
                                      PRIV_AUTHORIZED_UPDATE);
  }

  for (std::vector<KeyDefinition::AuthorizationData>::const_iterator auth_it =
           key_def.authorization_data.begin();
       auth_it != key_def.authorization_data.end(); ++auth_it) {
    KeyAuthorizationData* auth_data = data->add_authorization_data();
    switch (auth_it->type) {
      case KeyDefinition::AuthorizationData::TYPE_HMACSHA256:
        auth_data->set_type(
            KeyAuthorizationData::KEY_AUTHORIZATION_TYPE_HMACSHA256);
        break;
      case KeyDefinition::AuthorizationData::TYPE_AES256CBC_HMACSHA256:
        auth_data->set_type(
            KeyAuthorizationData::KEY_AUTHORIZATION_TYPE_AES256CBC_HMACSHA256);
        break;
      default:
        NOTREACHED();
        break;
    }

    for (std::vector<KeyDefinition::AuthorizationData::Secret>::const_iterator
             secret_it = auth_it->secrets.begin();
         secret_it != auth_it->secrets.end(); ++secret_it) {
      KeyAuthorizationSecret* secret = auth_data->add_secrets();
      secret->mutable_usage()->set_encrypt(secret_it->encrypt);
      secret->mutable_usage()->set_sign(secret_it->sign);
      if (!secret_it->symmetric_key.empty())
        secret->set_symmetric_key(secret_it->symmetric_key);
      if (!secret_it->public_key.empty())
        secret->set_public_key(secret_it->public_key);
      secret->set_wrapped(secret_it->wrapped);
    }
  }

  for (std::vector<KeyDefinition::ProviderData>::const_iterator it =
           key_def.provider_data.begin();
       it != key_def.provider_data.end(); ++it) {
    KeyProviderData::Entry* entry = data->mutable_provider_data()->add_entry();
    entry->set_name(it->name);
    if (it->number)
      entry->set_number(*it->number);
    if (it->bytes)
      entry->set_bytes(*it->bytes);
  }
}

MountError CryptohomeErrorToMountError(CryptohomeErrorCode code) {
  switch (code) {
    case CRYPTOHOME_ERROR_NOT_SET:
      return MOUNT_ERROR_NONE;
    case CRYPTOHOME_ERROR_ACCOUNT_NOT_FOUND:
      return MOUNT_ERROR_USER_DOES_NOT_EXIST;
    case CRYPTOHOME_ERROR_NOT_IMPLEMENTED:
    case CRYPTOHOME_ERROR_MOUNT_FATAL:
    case CRYPTOHOME_ERROR_KEY_QUOTA_EXCEEDED:
    case CRYPTOHOME_ERROR_BACKING_STORE_FAILURE:
      return MOUNT_ERROR_FATAL;
    case CRYPTOHOME_ERROR_AUTHORIZATION_KEY_NOT_FOUND:
    case CRYPTOHOME_ERROR_KEY_NOT_FOUND:
    case CRYPTOHOME_ERROR_AUTHORIZATION_KEY_FAILED:
      return MOUNT_ERROR_KEY_FAILURE;
    case CRYPTOHOME_ERROR_TPM_COMM_ERROR:
      return MOUNT_ERROR_TPM_COMM_ERROR;
    case CRYPTOHOME_ERROR_TPM_DEFEND_LOCK:
      return MOUNT_ERROR_TPM_DEFEND_LOCK;
    case CRYPTOHOME_ERROR_MOUNT_MOUNT_POINT_BUSY:
      return MOUNT_ERROR_MOUNT_POINT_BUSY;
    case CRYPTOHOME_ERROR_TPM_NEEDS_REBOOT:
      return MOUNT_ERROR_TPM_NEEDS_REBOOT;
    case CRYPTOHOME_ERROR_AUTHORIZATION_KEY_DENIED:
    case CRYPTOHOME_ERROR_KEY_LABEL_EXISTS:
    case CRYPTOHOME_ERROR_UPDATE_SIGNATURE_INVALID:
      return MOUNT_ERROR_KEY_FAILURE;
    case CRYPTOHOME_ERROR_MOUNT_OLD_ENCRYPTION:
      return MOUNT_ERROR_OLD_ENCRYPTION;
    case CRYPTOHOME_ERROR_MOUNT_PREVIOUS_MIGRATION_INCOMPLETE:
      return MOUNT_ERROR_PREVIOUS_MIGRATION_INCOMPLETE;
    default:
      NOTREACHED();
      return MOUNT_ERROR_FATAL;
  }
}

void KeyAuthorizationDataToAuthorizationData(
    const KeyAuthorizationData& authorization_data_proto,
    KeyDefinition::AuthorizationData* authorization_data) {
  switch (authorization_data_proto.type()) {
    case KeyAuthorizationData::KEY_AUTHORIZATION_TYPE_HMACSHA256:
      authorization_data->type =
          KeyDefinition::AuthorizationData::TYPE_HMACSHA256;
      break;
    case KeyAuthorizationData::KEY_AUTHORIZATION_TYPE_AES256CBC_HMACSHA256:
      authorization_data->type =
          KeyDefinition::AuthorizationData::TYPE_AES256CBC_HMACSHA256;
      break;
    default:
      NOTREACHED();
      return;
  }

  for (RepeatedPtrField<KeyAuthorizationSecret>::const_iterator it =
           authorization_data_proto.secrets().begin();
       it != authorization_data_proto.secrets().end(); ++it) {
    authorization_data->secrets.push_back(
        KeyDefinition::AuthorizationData::Secret(
            it->usage().encrypt(), it->usage().sign(), it->symmetric_key(),
            it->public_key(), it->wrapped()));
  }
}

}  // namespace cryptohome