// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_register.h"

#include <list>
#include <utility>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/cbor/cbor_writer.h"
#include "device/u2f/attestation_object.h"
#include "device/u2f/attested_credential_data.h"
#include "device/u2f/authenticator_data.h"
#include "device/u2f/ec_public_key.h"
#include "device/u2f/fido_attestation_statement.h"
#include "device/u2f/mock_u2f_device.h"
#include "device/u2f/mock_u2f_discovery.h"
#include "device/u2f/register_response_data.h"
#include "device/u2f/u2f_parsing_utils.h"
#include "device/u2f/u2f_response_test_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

using ::testing::_;

namespace {

// Helpers for testing U2f register responses.
std::vector<uint8_t> GetTestECPublicKeyCBOR() {
  std::vector<uint8_t> data(std::begin(kTestECPublicKeyCBOR),
                            std::end(kTestECPublicKeyCBOR));
  return data;
}

std::vector<uint8_t> GetTestRegisterResponse() {
  std::vector<uint8_t> data(std::begin(kTestU2fRegisterResponse),
                            std::end(kTestU2fRegisterResponse));
  return data;
}

std::vector<uint8_t> GetTestCredentialRawIdBytes() {
  std::vector<uint8_t> data(std::begin(kTestCredentialRawIdBytes),
                            std::end(kTestCredentialRawIdBytes));
  return data;
}

std::vector<uint8_t> GetU2fAttestationStatementCBOR() {
  std::vector<uint8_t> data(std::begin(kU2fAttestationStatementCBOR),
                            std::end(kU2fAttestationStatementCBOR));
  return data;
}

std::vector<uint8_t> GetTestAttestedCredentialDataBytes() {
  // Combine kTestAttestedCredentialDataPrefix and kTestECPublicKeyCBOR.
  std::vector<uint8_t> test_attested_data(
      std::begin(kTestAttestedCredentialDataPrefix),
      std::end(kTestAttestedCredentialDataPrefix));
  test_attested_data.insert(test_attested_data.end(),
                            std::begin(kTestECPublicKeyCBOR),
                            std::end(kTestECPublicKeyCBOR));
  return test_attested_data;
}

std::vector<uint8_t> GetTestAuthenticatorDataBytes() {
  // Build the test authenticator data.
  std::vector<uint8_t> test_authenticator_data(
      std::begin(kTestAuthenticatorDataPrefix),
      std::end(kTestAuthenticatorDataPrefix));
  std::vector<uint8_t> test_attested_data =
      GetTestAttestedCredentialDataBytes();
  test_authenticator_data.insert(test_authenticator_data.end(),
                                 test_attested_data.begin(),
                                 test_attested_data.end());
  return test_authenticator_data;
}

std::vector<uint8_t> GetTestAttestationObjectBytes() {
  std::vector<uint8_t> test_authenticator_object(std::begin(kFormatFidoU2fCBOR),
                                                 std::end(kFormatFidoU2fCBOR));
  test_authenticator_object.insert(test_authenticator_object.end(),
                                   std::begin(kAttStmtCBOR),
                                   std::end(kAttStmtCBOR));
  test_authenticator_object.insert(test_authenticator_object.end(),
                                   std::begin(kU2fAttestationStatementCBOR),
                                   std::end(kU2fAttestationStatementCBOR));
  test_authenticator_object.insert(test_authenticator_object.end(),
                                   std::begin(kAuthDataCBOR),
                                   std::end(kAuthDataCBOR));
  std::vector<uint8_t> test_authenticator_data =
      GetTestAuthenticatorDataBytes();
  test_authenticator_object.insert(test_authenticator_object.end(),
                                   test_authenticator_data.begin(),
                                   test_authenticator_data.end());
  return test_authenticator_object;
}

}  // namespace

class U2fRegisterTest : public testing::Test {
 public:
  U2fRegisterTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

class TestRegisterCallback {
 public:
  TestRegisterCallback()
      : callback_(base::Bind(&TestRegisterCallback::ReceivedCallback,
                             base::Unretained(this))) {}
  ~TestRegisterCallback() = default;

  void ReceivedCallback(U2fReturnCode status_code,
                        std::unique_ptr<RegisterResponseData> response_data) {
    response_ = std::make_pair(status_code, std::move(response_data));
    closure_.Run();
  }

  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
  WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

  const U2fRegister::RegisterResponseCallback& callback() { return callback_; }

 private:
  std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>> response_;
  base::Closure closure_;
  U2fRegister::RegisterResponseCallback callback_;
  base::RunLoop run_loop_;
};

TEST_F(U2fRegisterTest, TestRegisterSuccess) {
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  MockU2fDiscovery discovery;

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device.get(), DeviceTransactPtr(_, _))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(discovery, Start())
      .WillOnce(
          testing::Invoke(&discovery, &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::vector<std::vector<uint8_t>> registration_keys;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      registration_keys, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      kTestRelyingPartyId, {&discovery}, cb.callback());
  request->Start();
  discovery.AddDevice(std::move(device));
  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
      response = cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  EXPECT_EQ(GetTestCredentialRawIdBytes(), std::get<1>(response)->raw_id());
}

TEST_F(U2fRegisterTest, TestDelayedSuccess) {
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  MockU2fDiscovery discovery;

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  // Go through the state machine twice before success
  EXPECT_CALL(*device.get(), DeviceTransactPtr(_, _))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWinkRef(_))
      .Times(2)
      .WillRepeatedly(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(discovery, Start())
      .WillOnce(
          testing::Invoke(&discovery, &MockU2fDiscovery::StartSuccessAsync));
  TestRegisterCallback cb;

  std::vector<std::vector<uint8_t>> registration_keys;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      registration_keys, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      kTestRelyingPartyId, {&discovery}, cb.callback());
  request->Start();
  discovery.AddDevice(std::move(device));
  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
      response = cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  EXPECT_EQ(GetTestCredentialRawIdBytes(), std::get<1>(response)->raw_id());
}

TEST_F(U2fRegisterTest, TestMultipleDevices) {
  // Second device will have a successful touch
  std::unique_ptr<MockU2fDevice> device0(new MockU2fDevice());
  std::unique_ptr<MockU2fDevice> device1(new MockU2fDevice());
  MockU2fDiscovery discovery;

  EXPECT_CALL(*device0.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device1.get(), GetId())
      .WillRepeatedly(testing::Return("device1"));
  EXPECT_CALL(*device0.get(), DeviceTransactPtr(_, _))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied));
  // One wink per device
  EXPECT_CALL(*device0.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*device1.get(), DeviceTransactPtr(_, _))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device1.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(discovery, Start())
      .WillOnce(
          testing::Invoke(&discovery, &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::vector<std::vector<uint8_t>> registration_keys;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      registration_keys, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      kTestRelyingPartyId, {&discovery}, cb.callback());
  request->Start();
  discovery.AddDevice(std::move(device0));
  discovery.AddDevice(std::move(device1));
  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
      response = cb.WaitForCallback();

  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  EXPECT_EQ(GetTestCredentialRawIdBytes(),
            std::get<1>(response).get()->raw_id());
}

// Tests a scenario where a single device is connected and registration call
// is received with three unknown key handles. We expect that three check only
// sign-in calls be processed before registration.
TEST_F(U2fRegisterTest, TestSingleDeviceRegistrationWithExclusionList) {
  std::vector<uint8_t> unknown_key0(32, 0xB);
  std::vector<uint8_t> unknown_key1(32, 0xC);
  std::vector<uint8_t> unknown_key2(32, 0xD);
  std::vector<std::vector<uint8_t>> handles = {unknown_key0, unknown_key1,
                                               unknown_key2};
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  MockU2fDiscovery discovery;

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  // DeviceTransact() will be called four times including three check
  // only sign-in calls and one registration call. For the first three calls,
  // device will invoke MockU2fDevice::WrongData as the authenticator did not
  // create the three key handles provided in the exclude list. At the fourth
  // call, MockU2fDevice::NoErrorRegister will be invoked after registration.
  EXPECT_CALL(*device.get(), DeviceTransactPtr(_, _))
      .Times(4)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  // TryWink() will be called twice. First during the check only sign-in. After
  // check only sign operation is complete, request state is changed to IDLE,
  // and TryWink() is called again before Register() is called.
  EXPECT_CALL(*device.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(discovery, Start())
      .WillOnce(
          testing::Invoke(&discovery, &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      kTestRelyingPartyId, {&discovery}, cb.callback());
  discovery.AddDevice(std::move(device));

  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
      response = cb.WaitForCallback();

  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  EXPECT_EQ(GetTestCredentialRawIdBytes(), std::get<1>(response)->raw_id());
}

// Tests a scenario where two devices are connected and registration call is
// received with three unknown key handles. We assume that user will proceed the
// registration with second device, "device1".
TEST_F(U2fRegisterTest, TestMultipleDeviceRegistrationWithExclusionList) {
  std::vector<uint8_t> unknown_key0(32, 0xB);
  std::vector<uint8_t> unknown_key1(32, 0xC);
  std::vector<uint8_t> unknown_key2(32, 0xD);
  std::vector<std::vector<uint8_t>> handles = {unknown_key0, unknown_key1,
                                               unknown_key2};
  std::unique_ptr<MockU2fDevice> device0(new MockU2fDevice());
  std::unique_ptr<MockU2fDevice> device1(new MockU2fDevice());
  MockU2fDiscovery discovery;

  EXPECT_CALL(*device0.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device1.get(), GetId())
      .WillRepeatedly(testing::Return("device1"));

  // DeviceTransact() will be called four times: three times to check for
  // duplicate key handles and once for registration. Since user
  // will register using "device1", the fourth call will invoke
  // MockU2fDevice::NotSatisfied.
  EXPECT_CALL(*device0.get(), DeviceTransactPtr(_, _))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied));
  // We assume that user registers with second device. Therefore, the fourth
  // DeviceTransact() will invoke MockU2fDevice::NoErrorRegister after
  // successful registration.
  EXPECT_CALL(*device1.get(), DeviceTransactPtr(_, _))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));

  // TryWink() will be called twice on both devices -- during check only
  // sign-in operation and during registration attempt.
  EXPECT_CALL(*device0.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*device1.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(discovery, Start())
      .WillOnce(
          testing::Invoke(&discovery, &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      kTestRelyingPartyId, {&discovery}, cb.callback());

  discovery.AddDevice(std::move(device0));
  discovery.AddDevice(std::move(device1));
  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
      response = cb.WaitForCallback();

  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  EXPECT_EQ(GetTestCredentialRawIdBytes(), std::get<1>(response)->raw_id());
}

// Tests a scenario where single device is connected and registration is called
// with a key in the exclude list that was created by this device. We assume
// that the duplicate key is the last key handle in the exclude list. Therefore,
// after duplicate key handle is found, the process is expected to terminate
// after calling bogus registration which checks for user presence.
TEST_F(U2fRegisterTest, TestSingleDeviceRegistrationWithDuplicateHandle) {
  std::vector<uint8_t> unknown_key0(32, 0xB);
  std::vector<uint8_t> unknown_key1(32, 0xC);
  std::vector<uint8_t> unknown_key2(32, 0xD);
  std::vector<uint8_t> duplicate_key(32, 0xA);

  std::vector<std::vector<uint8_t>> handles = {unknown_key0, unknown_key1,
                                               unknown_key2, duplicate_key};
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  MockU2fDiscovery discovery;

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));

  // For four keys in exclude list, the first three keys will invoke
  // MockU2fDevice::WrongData and the final duplicate key handle will invoke
  // MockU2fDevice::NoErrorSign. Once duplicate key handle is found, bogus
  // registration is called to confirm user presence. This invokes
  // MockU2fDevice::NoErrorRegister.
  EXPECT_CALL(*device.get(), DeviceTransactPtr(_, _))
      .Times(5)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorSign))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));

  // Since duplicate key handle is found, registration process is terminated
  // before actual Register() is called on the device. Therefore, TryWink() is
  // invoked once.
  EXPECT_CALL(*device.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(discovery, Start())
      .WillOnce(
          testing::Invoke(&discovery, &MockU2fDiscovery::StartSuccessAsync));
  TestRegisterCallback cb;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      kTestRelyingPartyId, {&discovery}, cb.callback());
  discovery.AddDevice(std::move(device));
  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
      response = cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::CONDITIONS_NOT_SATISFIED, std::get<0>(response));
}

// Tests a scenario where one(device1) of the two devices connected has created
// a key handle provided in exclude list. We assume that duplicate key is the
// fourth key handle provided in the exclude list.
TEST_F(U2fRegisterTest, TestMultipleDeviceRegistrationWithDuplicateHandle) {
  std::vector<uint8_t> unknown_key0(32, 0xB);
  std::vector<uint8_t> unknown_key1(32, 0xC);
  std::vector<uint8_t> unknown_key2(32, 0xD);
  std::vector<uint8_t> duplicate_key(32, 0xA);
  std::vector<std::vector<uint8_t>> handles = {unknown_key0, unknown_key1,
                                               unknown_key2, duplicate_key};
  std::unique_ptr<MockU2fDevice> device0(new MockU2fDevice());
  std::unique_ptr<MockU2fDevice> device1(new MockU2fDevice());
  MockU2fDiscovery discovery;

  EXPECT_CALL(*device0.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));

  EXPECT_CALL(*device1.get(), GetId())
      .WillRepeatedly(testing::Return("device1"));

  // Since the first device did not create any of the key handles provided in
  // exclude list, we expect that check only sign() should be called
  // four times, and all the calls to DeviceTransact() invoke
  // MockU2fDevice::WrongData.
  EXPECT_CALL(*device0.get(), DeviceTransactPtr(_, _))
      .Times(4)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData));
  // Since the last key handle in exclude list is a duplicate key, we expect
  // that the first three calls to check only sign() invoke
  // MockU2fDevice::WrongData and that fourth sign() call invoke
  // MockU2fDevice::NoErrorSign. After duplicate key is found, process is
  // terminated after user presence is verified using bogus registration, which
  // invokes MockU2fDevice::NoErrorRegister.
  EXPECT_CALL(*device1.get(), DeviceTransactPtr(_, _))
      .Times(5)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorSign))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));

  EXPECT_CALL(*device0.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(*device1.get(), TryWinkRef(_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(discovery, Start())
      .WillOnce(
          testing::Invoke(&discovery, &MockU2fDiscovery::StartSuccessAsync));
  TestRegisterCallback cb;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      kTestRelyingPartyId, {&discovery}, cb.callback());
  discovery.AddDevice(std::move(device0));
  discovery.AddDevice(std::move(device1));
  const std::pair<U2fReturnCode, std::unique_ptr<RegisterResponseData>>&
      response = cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::CONDITIONS_NOT_SATISFIED, std::get<0>(response));
}

// These test the parsing of the U2F raw bytes of the registration response.
// Test that an EC public key serializes to CBOR properly.
TEST_F(U2fRegisterTest, TestSerializedPublicKey) {
  std::unique_ptr<ECPublicKey> public_key =
      ECPublicKey::ExtractFromU2fRegistrationResponse(
          u2f_parsing_utils::kEs256, GetTestRegisterResponse());
  EXPECT_EQ(GetTestECPublicKeyCBOR(), public_key->EncodeAsCBOR());
}

// Test that the attestation statement cbor map is constructed properly.
TEST_F(U2fRegisterTest, TestU2fAttestationStatementCBOR) {
  std::unique_ptr<FidoAttestationStatement> fido_attestation_statement =
      FidoAttestationStatement::CreateFromU2fRegisterResponse(
          GetTestRegisterResponse());
  auto cbor = cbor::CBORWriter::Write(
      cbor::CBORValue(fido_attestation_statement->GetAsCBORMap()));
  ASSERT_TRUE(cbor.has_value());
  EXPECT_EQ(GetU2fAttestationStatementCBOR(), cbor.value());
}

// Tests that well-formed attested credential data serializes properly.
TEST_F(U2fRegisterTest, TestAttestedCredentialData) {
  std::unique_ptr<ECPublicKey> public_key =
      ECPublicKey::ExtractFromU2fRegistrationResponse(
          u2f_parsing_utils::kEs256, GetTestRegisterResponse());
  AttestedCredentialData attested_data =
      AttestedCredentialData::CreateFromU2fRegisterResponse(
          GetTestRegisterResponse(), std::vector<uint8_t>(16, 0) /* aaguid */,
          std::move(public_key));

  EXPECT_EQ(kTestAttestedData, attested_data.SerializeAsBytes());
}

// Tests that well-formed authenticator data serializes properly.
TEST_F(U2fRegisterTest, TestAuthenticatorData) {
  std::unique_ptr<ECPublicKey> public_key =
      ECPublicKey::ExtractFromU2fRegistrationResponse(
          u2f_parsing_utils::kEs256, GetTestRegisterResponse());
  AttestedCredentialData attested_data =
      AttestedCredentialData::CreateFromU2fRegisterResponse(
          GetTestRegisterResponse(), std::vector<uint8_t>(16, 0) /* aaguid */,
          std::move(public_key));

  AuthenticatorData::Flags flags =
      static_cast<AuthenticatorData::Flags>(
          AuthenticatorData::Flag::TEST_OF_USER_PRESENCE) |
      static_cast<AuthenticatorData::Flags>(
          AuthenticatorData::Flag::ATTESTATION);

  std::unique_ptr<AuthenticatorData> authenticator_data =
      AuthenticatorData::Create(kTestRelyingPartyId, flags,
                                std::vector<uint8_t>(4, 0) /* counter */,
                                attested_data);

  EXPECT_EQ(kTestAuthenticatorData, authenticator_data->SerializeToByteArray());
}

// Tests that a U2F attestation object serializes properly.
TEST_F(U2fRegisterTest, TestU2fAttestationObject) {
  std::unique_ptr<ECPublicKey> public_key =
      ECPublicKey::ExtractFromU2fRegistrationResponse(
          u2f_parsing_utils::kEs256, GetTestRegisterResponse());
  AttestedCredentialData attested_data =
      AttestedCredentialData::CreateFromU2fRegisterResponse(
          GetTestRegisterResponse(), std::vector<uint8_t>(16, 0) /* aaguid */,
          std::move(public_key));

  AuthenticatorData::Flags flags =
      static_cast<AuthenticatorData::Flags>(
          AuthenticatorData::Flag::TEST_OF_USER_PRESENCE) |
      static_cast<AuthenticatorData::Flags>(
          AuthenticatorData::Flag::ATTESTATION);
  std::unique_ptr<AuthenticatorData> authenticator_data =
      AuthenticatorData::Create(kTestRelyingPartyId, flags,
                                std::vector<uint8_t>(4, 0) /* counter */,
                                attested_data);

  // Construct the attestation statement.
  std::unique_ptr<FidoAttestationStatement> fido_attestation_statement =
      FidoAttestationStatement::CreateFromU2fRegisterResponse(
          GetTestRegisterResponse());

  // Construct the attestation object.
  auto attestation_object = std::make_unique<AttestationObject>(
      std::move(authenticator_data), std::move(fido_attestation_statement));

  EXPECT_EQ(kTestAttestationObject,
            attestation_object->SerializeToCBOREncodedBytes());
}

// Test that a U2F register response is properly parsed.
TEST_F(U2fRegisterTest, TestRegisterResponseData) {
  std::unique_ptr<RegisterResponseData> response =
      RegisterResponseData::CreateFromU2fRegisterResponse(
          kTestRelyingPartyId, GetTestRegisterResponse());
  EXPECT_EQ(GetTestCredentialRawIdBytes(), response->raw_id());
  EXPECT_EQ(kTestAttestationObject,
            response->GetCBOREncodedAttestationObject());
}

}  // namespace device
