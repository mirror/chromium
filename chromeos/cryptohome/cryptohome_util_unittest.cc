// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/cryptohome/cryptohome_util.h"

#include <string>

#include "base/optional.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome/key.pb.h"
#include "chromeos/dbus/cryptohome/rpc.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptohome {

TEST(CryptohomeUtilTest, CreateAuthorizationRequestEmptyLabel) {
  const std::string expected_secret = "secret";

  const AuthorizationRequest auth_request =
      CreateAuthorizationRequest(std::string(), expected_secret);

  EXPECT_FALSE(auth_request.key().data().has_label());
  EXPECT_EQ(auth_request.key().secret(), expected_secret);
}

TEST(CryptohomeUtilTest, CreateAuthorizationRequestWithLabel) {
  const std::string expected_label = "some_label";
  const std::string expected_secret = "some_secret";

  const AuthorizationRequest auth_request =
      CreateAuthorizationRequest(expected_label, expected_secret);

  EXPECT_EQ(auth_request.key().data().label(), expected_label);
  EXPECT_EQ(auth_request.key().secret(), expected_secret);
}

TEST(CryptohomeUtilTest, BaseReplyToMountErrorNullOptional) {
  const base::Optional<BaseReply> reply = base::nullopt;

  MountError actual_error = BaseReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_FATAL);
}

TEST(CryptohomeUtilTest, BaseReplyToMountErrorEmptyReply) {
  BaseReply result;
  const base::Optional<BaseReply> reply = std::move(result);

  MountError actual_error = BaseReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_NONE);
}

TEST(CryptohomeUtilTest, BaseReplyToMountErrorNoError) {
  BaseReply result;
  result.set_error(CRYPTOHOME_ERROR_NOT_SET);
  const base::Optional<BaseReply> reply = std::move(result);

  MountError actual_error = BaseReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_NONE);
}

TEST(CryptohomeUtilTest, BaseReplyToMountErrorAuthFailure) {
  BaseReply result;
  result.set_error(CRYPTOHOME_ERROR_AUTHORIZATION_KEY_NOT_FOUND);
  const base::Optional<BaseReply> reply = std::move(result);

  MountError actual_error = BaseReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_KEY_FAILURE);
}

TEST(CryptohomeUtilTest, MountExReplyToMountErrorNullOptional) {
  const base::Optional<BaseReply> reply = base::nullopt;

  MountError actual_error = MountExReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_FATAL);
}

TEST(CryptohomeUtilTest, MountExReplyToMountErrorEmptyReply) {
  BaseReply result;
  const base::Optional<BaseReply> reply = std::move(result);

  MountError actual_error = MountExReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_FATAL);
}

TEST(CryptohomeUtilTest, MountExReplyToMountErrorNoExtension) {
  BaseReply result;
  result.set_error(CRYPTOHOME_ERROR_NOT_SET);
  const base::Optional<BaseReply> reply = std::move(result);

  MountError actual_error = MountExReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_FATAL);
}

TEST(CryptohomeUtilTest, MountExReplyToMountErrorNoError) {
  BaseReply result;
  result.set_error(CRYPTOHOME_ERROR_NOT_SET);
  result.MutableExtension(MountReply::reply);
  const base::Optional<BaseReply> reply = std::move(result);

  MountError actual_error = MountExReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_NONE);
}

TEST(CryptohomeUtilTest, MountExReplyToMountErrorAuthFailure) {
  BaseReply result;
  result.set_error(CRYPTOHOME_ERROR_AUTHORIZATION_KEY_NOT_FOUND);
  result.MutableExtension(MountReply::reply);
  const base::Optional<BaseReply> reply = std::move(result);

  MountError actual_error = MountExReplyToMountError(reply);

  EXPECT_EQ(actual_error, MOUNT_ERROR_KEY_FAILURE);
}

TEST(CryptohomeUtilTest, MountExReplyToMountHash) {
  const std::string expected_hash = "username";
  BaseReply result;
  result.set_error(CRYPTOHOME_ERROR_NOT_SET);
  MountReply* mount = result.MutableExtension(MountReply::reply);
  mount->set_sanitized_username(expected_hash);

  const std::string actual_hash = MountExReplyToMountHash(result);

  EXPECT_EQ(actual_hash, expected_hash);
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyType) {
  Key key;

  KeyDefinitionToKey(KeyDefinition(), &key);

  EXPECT_EQ(key.data().type(), KeyData::KEY_TYPE_PASSWORD);
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeySecret) {
  const std::string expected_secret = "my_dog_ate_my_homework";
  KeyDefinition key_def;
  key_def.secret = expected_secret;
  Key key;

  KeyDefinitionToKey(key_def, &key);

  EXPECT_EQ(key.secret(), expected_secret);
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyLabel) {
  const std::string expected_label = "millenials hate labels";
  KeyDefinition key_def;
  key_def.label = expected_label;
  Key key;

  KeyDefinitionToKey(key_def, &key);

  EXPECT_EQ(key.data().label(), expected_label);
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyNonpositiveRevision) {
  KeyDefinition key_def;
  key_def.revision = -1;
  Key key;

  KeyDefinitionToKey(key_def, &key);

  EXPECT_EQ(key.data().revision(), 0);
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyPositiveRevision) {
  constexpr int expected_revision = 10;
  KeyDefinition key_def;
  key_def.revision = expected_revision;
  Key key;

  KeyDefinitionToKey(key_def, &key);

  EXPECT_EQ(key.data().revision(), expected_revision);
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyDefaultPrivileges) {
  KeyDefinition key_def;
  Key key;

  KeyDefinitionToKey(key_def, &key);
  KeyPrivileges privileges = key.data().privileges();

  EXPECT_TRUE(privileges.mount());
  EXPECT_TRUE(privileges.add());
  EXPECT_TRUE(privileges.remove());
  EXPECT_TRUE(privileges.update());
  EXPECT_FALSE(privileges.authorized_update());
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyMountPrivileges) {
  KeyDefinition key_def;
  key_def.privileges = PRIV_MOUNT;
  Key key;

  KeyDefinitionToKey(key_def, &key);
  KeyPrivileges privileges = key.data().privileges();

  EXPECT_TRUE(privileges.mount());
  EXPECT_FALSE(privileges.add());
  EXPECT_FALSE(privileges.remove());
  EXPECT_FALSE(privileges.update());
  EXPECT_FALSE(privileges.authorized_update());
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyAddPrivileges) {
  KeyDefinition key_def;
  key_def.privileges = PRIV_ADD;
  Key key;

  KeyDefinitionToKey(key_def, &key);
  KeyPrivileges privileges = key.data().privileges();

  EXPECT_FALSE(privileges.mount());
  EXPECT_TRUE(privileges.add());
  EXPECT_FALSE(privileges.remove());
  EXPECT_FALSE(privileges.update());
  EXPECT_FALSE(privileges.authorized_update());
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyRemovePrivileges) {
  KeyDefinition key_def;
  key_def.privileges = PRIV_REMOVE;
  Key key;

  KeyDefinitionToKey(key_def, &key);
  KeyPrivileges privileges = key.data().privileges();

  EXPECT_FALSE(privileges.mount());
  EXPECT_FALSE(privileges.add());
  EXPECT_TRUE(privileges.remove());
  EXPECT_FALSE(privileges.update());
  EXPECT_FALSE(privileges.authorized_update());
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyUpdatePrivileges) {
  KeyDefinition key_def;
  key_def.privileges = PRIV_MIGRATE;
  Key key;

  KeyDefinitionToKey(key_def, &key);
  KeyPrivileges privileges = key.data().privileges();

  EXPECT_FALSE(privileges.mount());
  EXPECT_FALSE(privileges.add());
  EXPECT_FALSE(privileges.remove());
  EXPECT_TRUE(privileges.update());
  EXPECT_FALSE(privileges.authorized_update());
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyAuthorizedUpdatePrivileges) {
  KeyDefinition key_def;
  key_def.privileges = PRIV_AUTHORIZED_UPDATE;
  Key key;

  KeyDefinitionToKey(key_def, &key);
  KeyPrivileges privileges = key.data().privileges();

  EXPECT_FALSE(privileges.mount());
  EXPECT_FALSE(privileges.add());
  EXPECT_FALSE(privileges.remove());
  EXPECT_FALSE(privileges.update());
  EXPECT_TRUE(privileges.authorized_update());
}

TEST(CryptohomeUtilTest, KeyDefinitionToKeyAllPrivileges) {
  KeyDefinition key_def;
  key_def.privileges = PRIV_DEFAULT | PRIV_AUTHORIZED_UPDATE;
  Key key;

  KeyDefinitionToKey(key_def, &key);
  KeyPrivileges privileges = key.data().privileges();

  EXPECT_TRUE(privileges.mount());
  EXPECT_TRUE(privileges.add());
  EXPECT_TRUE(privileges.remove());
  EXPECT_TRUE(privileges.update());
  EXPECT_TRUE(privileges.authorized_update());
}

TEST(CryptohomeUtilTest, KeyAuthorizationDataToAuthorizationDataHmacSha256) {
  KeyAuthorizationData auth_data_proto;
  auth_data_proto.set_type(
      KeyAuthorizationData::KEY_AUTHORIZATION_TYPE_HMACSHA256);
  KeyDefinition::AuthorizationData auth_data;

  KeyAuthorizationDataToAuthorizationData(auth_data_proto, &auth_data);

  EXPECT_EQ(auth_data.type, KeyDefinition::AuthorizationData::TYPE_HMACSHA256);
}

TEST(CryptohomeUtilTest, KeyAuthorizationDataToAuthorizationDataAes256Cbc) {
  KeyAuthorizationData auth_data_proto;
  auth_data_proto.set_type(
      KeyAuthorizationData::KEY_AUTHORIZATION_TYPE_AES256CBC_HMACSHA256);
  KeyDefinition::AuthorizationData auth_data;

  KeyAuthorizationDataToAuthorizationData(auth_data_proto, &auth_data);

  EXPECT_EQ(auth_data.type,
            KeyDefinition::AuthorizationData::TYPE_AES256CBC_HMACSHA256);
}

TEST(CryptohomeUtilTest, KeyAuthorizationDataToAuthorizationDataSecret) {
  constexpr bool encrypt = true;
  constexpr bool sign = false;
  constexpr bool wrapped = true;
  const std::string symmetric_key = "symmetric_key";
  const std::string public_key = "public_key";
  KeyAuthorizationData auth_data_proto;
  KeyAuthorizationSecret* secret = auth_data_proto.add_secrets();
  KeyAuthorizationSecretUsage* usage = secret->mutable_usage();
  usage->set_encrypt(encrypt);
  usage->set_sign(sign);
  secret->set_wrapped(wrapped);
  secret->set_symmetric_key(symmetric_key);
  secret->set_public_key(public_key);
  KeyDefinition::AuthorizationData::Secret expected_secret(
      encrypt, sign, symmetric_key, public_key, wrapped);
  KeyDefinition::AuthorizationData auth_data;

  KeyAuthorizationDataToAuthorizationData(auth_data_proto, &auth_data);

  EXPECT_EQ(auth_data.secrets.back(), expected_secret);
}

}  // namespace cryptohome
