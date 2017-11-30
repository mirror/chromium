// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/intel_csme_cdm_proxy.h"

#include <d3d11.h>
#include <d3d11_1.h>
#include <initguid.h>

#include "base/bind.h"
#include "media/gpu/d3d11_mocks.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::Ne;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::WithArgs;
using ::testing::_;

namespace media {

// TODO(rkuroiwa): Find out a way to dedup this. Simply putting this in the
// header file doesn't work.
DEFINE_GUID(D3D11_CONFIG_WIDEVINE_STREAM_ID,
            0x586e681,
            0x4e14,
            0x4133,
            0x85,
            0xe5,
            0xa1,
            0x4,
            0x1f,
            0x59,
            0x9e,
            0x26);

namespace {
// Use this function to create a mock so that they are ref-counted correctly.
template <typename Interface>
Microsoft::WRL::ComPtr<Interface> CreateMock() {
  Interface* mock = new Interface();
  mock->AddRef();
  return mock;
}
}  // namespace

class D3D11CreateDeviceMock {
 public:
  MOCK_METHOD10(Create,
                HRESULT(IDXGIAdapter* adapter,
                        D3D_DRIVER_TYPE driver_type,
                        HMODULE software,
                        UINT flags,
                        const D3D_FEATURE_LEVEL* feature_levels,
                        UINT feature_level,
                        UINT sdk_version,
                        ID3D11Device** device_out,
                        D3D_FEATURE_LEVEL* feature_level_out,
                        ID3D11DeviceContext** immediate_context_out));
};

// Class for mocking the callbacks that get passed to the proxy methods.
class CallbackMock {
 public:
  MOCK_METHOD3(InitializeCallback, CdmProxy::InitializeCB::RunType);
  MOCK_METHOD2(ProcessCallback, CdmProxy::ProcessCB::RunType);
  MOCK_METHOD3(CreateMediaCryptoSessionCallback,
               CdmProxy::CreateMediaCryptoSessionCB::RunType);
};

class IntelCSMECdmProxyTest : public ::testing::Test {
 protected:
  void InjectMockD3D11CreateDeviceFunction() {
    proxy_.create_device_func_ = base::BindRepeating(
        &D3D11CreateDeviceMock::Create, base::Unretained(&create_device_mock_));
  }

  // Helper method to do Initialize(). Only useful if the test doesn't require
  // access to the mocks later.
  void Initialize(CdmProxy::InitializeCB callback) {
    InjectMockD3D11CreateDeviceFunction();
    // These are ComPtr which are refcounted, so the ComPtrs held in the
    // instance are still safe to use even if these variables go out of scope.
    auto device_mock = CreateMock<D3D11DeviceMock>();
    auto video_device_mock = CreateMock<D3D11VideoDeviceMock>();
    auto video_device1_mock = CreateMock<D3D11VideoDevice1Mock>();
    auto crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
    auto device_context_mock = CreateMock<D3D11DeviceContextMock>();
    auto video_context_mock = CreateMock<D3D11VideoContextMock>();
    auto video_context1_mock = CreateMock<D3D11VideoContext1Mock>();

    EXPECT_CALL(create_device_mock_,
                Create(_, D3D_DRIVER_TYPE_HARDWARE, _, _, _, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<7>(device_mock.Get()),
                        SetArgPointee<8>(D3D_FEATURE_LEVEL_11_1),
                        SetArgPointee<9>(device_context_mock.Get()),
                        Return(S_OK)));
    EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice, _))
        .Times(AtLeast(1))
        .WillOnce(
            DoAll(SetArgPointee<1>(video_device_mock.Get()), Return(S_OK)));

    EXPECT_CALL(*video_device_mock.Get(), CreateCryptoSession(_, _, _, _))
        .WillOnce(
            DoAll(SetArgPointee<3>(crypto_session_mock.Get()), Return(S_OK)));

    EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice1, _))
        .Times(AtLeast(1))
        .WillOnce(
            DoAll(SetArgPointee<1>(video_device1_mock.Get()), Return(S_OK)));

    EXPECT_CALL(*device_context_mock.Get(),
                QueryInterface(IID_ID3D11VideoContext, _))
        .WillOnce(
            DoAll(SetArgPointee<1>(video_context_mock.Get()), Return(S_OK)));

    EXPECT_CALL(*device_context_mock.Get(),
                QueryInterface(IID_ID3D11VideoContext1, _))
        .WillOnce(
            DoAll(SetArgPointee<1>(video_context1_mock.Get()), Return(S_OK)));

    EXPECT_CALL(*video_device1_mock.Get(),
                GetCryptoSessionPrivateDataSize(
                    Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _, _, _, _))
        .WillOnce(Return(S_OK));

    proxy_.Initialize(nullptr, std::move(callback));
  }

  IntelCSMECdmProxy proxy_;
  D3D11CreateDeviceMock create_device_mock_;
  CallbackMock callback_mock_;
};

// Verifies that if device creation fails, then the call fails.
TEST_F(IntelCSMECdmProxyTest, FailedToCreateDevice) {
  InjectMockD3D11CreateDeviceFunction();
  EXPECT_CALL(create_device_mock_, Create(_, _, _, _, _, _, _, _, _, _))
      .WillOnce(Return(E_FAIL));
  EXPECT_CALL(callback_mock_,
              InitializeCallback(CdmProxy::Status::kFail, _, _));
  proxy_.Initialize(nullptr, base::BindOnce(&CallbackMock::InitializeCallback,
                                            base::Unretained(&callback_mock_)));
}

// Initialize() success case.
TEST_F(IntelCSMECdmProxyTest, Initialize) {
  EXPECT_CALL(callback_mock_, InitializeCallback(CdmProxy::Status::kOk, _, _));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
}

// Verifies that Process() won't work if not initialized.
TEST_F(IntelCSMECdmProxyTest, ProcessUninitialized) {
  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  // Output size is also arbitrary, just has to match with the mock.
  const uint32_t kExpectedOutputDataSize = 20;
  EXPECT_CALL(callback_mock_, ProcessCallback(CdmProxy::Status::kFail, _));
  proxy_.Process(CdmProxy::Function::kIntelNegotiateCryptoSessionKeyExchange, 0,
                 kAnyInput, kExpectedOutputDataSize,
                 base::BindOnce(&CallbackMock::ProcessCallback,
                                base::Unretained(&callback_mock_)));
}

// Verifies that using a crypto session that is not reported will fail.
TEST_F(IntelCSMECdmProxyTest, ProcessInvalidCryptoSessionID) {
  uint32_t crypto_session_id = 0;
  EXPECT_CALL(callback_mock_, InitializeCallback(CdmProxy::Status::kOk, _, _))
      .WillOnce(SaveArg<2>(&crypto_session_id));
  ASSERT_NO_FATAL_FAILURE(Initialize(base::BindOnce(
      &CallbackMock::InitializeCallback, base::Unretained(&callback_mock_))));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  // Output size is also arbitrary, just has to match with the mock.
  const uint32_t kExpectedOutputDataSize = 20;
  EXPECT_CALL(callback_mock_, ProcessCallback(CdmProxy::Status::kFail, _));

  // Use a crypto session ID that hasn't been reported.
  proxy_.Process(CdmProxy::Function::kIntelNegotiateCryptoSessionKeyExchange,
                 crypto_session_id + 1, kAnyInput, kExpectedOutputDataSize,
                 base::BindOnce(&CallbackMock::ProcessCallback,
                                base::Unretained(&callback_mock_)));
}

MATCHER_P2(MatchesKeyExchangeStructure, expected, input_struct_size, "") {
  D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA* actual =
      static_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA*>(arg);
  if (expected->HWProtectionFunctionID != actual->HWProtectionFunctionID) {
    *result_listener << "function IDs mismatch. Expected "
                     << expected->HWProtectionFunctionID << " actual "
                     << actual->HWProtectionFunctionID;
    return false;
  }
  D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA* expected_input_data =
      expected->pInputData;
  D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA* actual_input_data =
      actual->pInputData;
  if (memcmp(expected_input_data, actual_input_data, input_struct_size) != 0) {
    *result_listener
        << "D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA don't match.";
    return false;
  }
  D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA* expected_output_data =
      expected->pOutputData;
  D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA* actual_output_data =
      actual->pOutputData;
  // Don't check that pbOutput field. It's filled by the callee.
  if (expected_output_data->PrivateDataSize !=
      actual_output_data->PrivateDataSize) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "PrivateDataSize don't match. Expected "
                     << expected_output_data->PrivateDataSize << " actual "
                     << actual_output_data->PrivateDataSize;
    return false;
  }
  if (expected_output_data->HWProtectionDataSize !=
      actual_output_data->HWProtectionDataSize) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "HWProtectionDataSize don't match. Expected "
                     << expected_output_data->HWProtectionDataSize << " actual "
                     << actual_output_data->HWProtectionDataSize;
    return false;
  }
  if (expected_output_data->TransportTime !=
      actual_output_data->TransportTime) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "TransportTime don't match. Expected "
                     << expected_output_data->TransportTime << " actual "
                     << actual_output_data->TransportTime;
    return false;
  }
  if (expected_output_data->ExecutionTime !=
      actual_output_data->ExecutionTime) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "ExecutionTime don't match. Expected "
                     << expected_output_data->ExecutionTime << " actual "
                     << actual_output_data->ExecutionTime;
    return false;
  }
  if (expected_output_data->MaxHWProtectionDataSize !=
      actual_output_data->MaxHWProtectionDataSize) {
    *result_listener << "D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA::"
                        "MaxHWProtectionDataSize don't match. Expected "
                     << expected_output_data->MaxHWProtectionDataSize
                     << " actual "
                     << actual_output_data->MaxHWProtectionDataSize;
    return false;
  }
  return true;
}

TEST_F(IntelCSMECdmProxyTest, Process) {
  InjectMockD3D11CreateDeviceFunction();
  auto device_mock = CreateMock<D3D11DeviceMock>();
  auto video_device_mock = CreateMock<D3D11VideoDeviceMock>();
  auto video_device1_mock = CreateMock<D3D11VideoDevice1Mock>();
  auto crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
  auto device_context_mock = CreateMock<D3D11DeviceContextMock>();
  auto video_context_mock = CreateMock<D3D11VideoContextMock>();
  auto video_context1_mock = CreateMock<D3D11VideoContext1Mock>();

  // These size values are arbitrary.
  const UINT kPrivateInputSize = 10;
  const UINT kPrivateOutputSize = 40;
  uint32_t crypto_session_id = 0;
  EXPECT_CALL(
      callback_mock_,
      InitializeCallback(
          CdmProxy::Status::kOk,
          CdmProxy::Protocol::kIntelConvergedSecurityAndManageabilityEngine, _))
      .WillOnce(SaveArg<2>(&crypto_session_id));
  EXPECT_CALL(create_device_mock_,
              Create(_, D3D_DRIVER_TYPE_HARDWARE, _, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<7>(device_mock.Get()),
                      SetArgPointee<8>(D3D_FEATURE_LEVEL_11_1),
                      SetArgPointee<9>(device_context_mock.Get()),
                      Return(S_OK)));
  EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<1>(video_device_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*video_device_mock.Get(), CreateCryptoSession(_, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<3>(crypto_session_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice1, _))
      .Times(AtLeast(1))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_device1_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_context_mock.Get(),
              QueryInterface(IID_ID3D11VideoContext, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_context_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_context_mock.Get(),
              QueryInterface(IID_ID3D11VideoContext1, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_context1_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*video_device1_mock.Get(),
              GetCryptoSessionPrivateDataSize(
                  Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<3>(kPrivateInputSize),
                      SetArgPointee<4>(kPrivateOutputSize), Return(S_OK)));

  proxy_.Initialize(nullptr, base::BindOnce(&CallbackMock::InitializeCallback,
                                            base::Unretained(&callback_mock_)));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  // Output size is also arbitrary, just has to match with the mock.
  const uint32_t kExpectedOutputDataSize = 20;

  const uint32_t input_structure_size =
      sizeof(D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA) - 4 +
      kAnyInput.size();
  const uint32_t output_structure_size =
      sizeof(D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA) - 4 +
      kExpectedOutputDataSize;
  std::unique_ptr<uint8_t[]> input_data_raw(new uint8_t[input_structure_size]);
  std::unique_ptr<uint8_t[]> output_data_raw(
      new uint8_t[output_structure_size]);

  D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA* input_data =
      reinterpret_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA*>(
          input_data_raw.get());
  D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA* output_data =
      reinterpret_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA*>(
          output_data_raw.get());

  D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA expected_key_exchange_data = {};
  expected_key_exchange_data.HWProtectionFunctionID = 0x90000001;
  expected_key_exchange_data.pInputData = input_data;
  expected_key_exchange_data.pOutputData = output_data;
  input_data->PrivateDataSize = kPrivateInputSize;
  input_data->HWProtectionDataSize = 0;
  memcpy(input_data->pbInput, kAnyInput.data(), kAnyInput.size());

  output_data->PrivateDataSize = kPrivateOutputSize;
  output_data->HWProtectionDataSize = 0;
  output_data->TransportTime = 0;
  output_data->ExecutionTime = 0;
  output_data->MaxHWProtectionDataSize = kExpectedOutputDataSize;

  // The value does not matter, so making non zero vector.
  std::vector<uint8_t> test_output_data(kExpectedOutputDataSize, 0xAA);
  EXPECT_CALL(callback_mock_, ProcessCallback(CdmProxy::Status::kOk, _));

  auto set_test_output_data = [&test_output_data](void* output) {
    D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA* kex_struct =
        static_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA*>(output);
    memcpy(kex_struct->pOutputData->pbOutput, test_output_data.data(),
           test_output_data.size());
  };

  EXPECT_CALL(*video_context_mock.Get(),
              NegotiateCryptoSessionKeyExchange(
                  _, sizeof(expected_key_exchange_data),
                  MatchesKeyExchangeStructure(&expected_key_exchange_data,
                                              input_structure_size)))
      .WillOnce(DoAll(WithArgs<2>(Invoke(set_test_output_data)), Return(S_OK)));

  proxy_.Process(CdmProxy::Function::kIntelNegotiateCryptoSessionKeyExchange,
                 crypto_session_id, kAnyInput, kExpectedOutputDataSize,
                 base::BindOnce(&CallbackMock::ProcessCallback,
                                base::Unretained(&callback_mock_)));
}

TEST_F(IntelCSMECdmProxyTest, CreateMediaCryptoSessionUninitialized) {
  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  EXPECT_CALL(callback_mock_,
              CreateMediaCryptoSessionCallback(CdmProxy::Status::kFail, _, _));
  proxy_.CreateMediaCryptoSession(
      kAnyInput, base::BindOnce(&CallbackMock::CreateMediaCryptoSessionCallback,
                                base::Unretained(&callback_mock_)));
}

// Tests the case where no extra data is specified. This is a success case.
TEST_F(IntelCSMECdmProxyTest, CreateMediaCryptoSessionNoExtraData) {
  InjectMockD3D11CreateDeviceFunction();
  auto device_mock = CreateMock<D3D11DeviceMock>();
  auto video_device_mock = CreateMock<D3D11VideoDeviceMock>();
  auto video_device1_mock = CreateMock<D3D11VideoDevice1Mock>();
  auto crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
  auto device_context_mock = CreateMock<D3D11DeviceContextMock>();
  auto video_context_mock = CreateMock<D3D11VideoContextMock>();
  auto video_context1_mock = CreateMock<D3D11VideoContext1Mock>();

  uint32_t crypto_session_id_from_initialize = 0;
  EXPECT_CALL(
      callback_mock_,
      InitializeCallback(
          CdmProxy::Status::kOk,
          CdmProxy::Protocol::kIntelConvergedSecurityAndManageabilityEngine, _))
      .WillOnce(SaveArg<2>(&crypto_session_id_from_initialize));
  EXPECT_CALL(create_device_mock_,
              Create(_, D3D_DRIVER_TYPE_HARDWARE, _, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<7>(device_mock.Get()),
                      SetArgPointee<8>(D3D_FEATURE_LEVEL_11_1),
                      SetArgPointee<9>(device_context_mock.Get()),
                      Return(S_OK)));
  EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<1>(video_device_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*video_device_mock.Get(), CreateCryptoSession(_, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<3>(crypto_session_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice1, _))
      .Times(AtLeast(1))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_device1_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_context_mock.Get(),
              QueryInterface(IID_ID3D11VideoContext, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_context_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_context_mock.Get(),
              QueryInterface(IID_ID3D11VideoContext1, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_context1_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*video_device1_mock.Get(),
              GetCryptoSessionPrivateDataSize(
                  Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _, _, _, _))
      .WillOnce(Return(S_OK));

  proxy_.Initialize(nullptr, base::BindOnce(&CallbackMock::InitializeCallback,
                                            base::Unretained(&callback_mock_)));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // Expect a new crypto session.
  EXPECT_CALL(callback_mock_, CreateMediaCryptoSessionCallback(
                                  CdmProxy::Status::kOk,
                                  Ne(crypto_session_id_from_initialize), _));
  auto media_crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
  EXPECT_CALL(*video_device_mock.Get(),
              CreateCryptoSession(Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _,
                                  Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _))
      .WillOnce(DoAll(SetArgPointee<3>(media_crypto_session_mock.Get()),
                      Return(S_OK)));

  EXPECT_CALL(*video_context1_mock.Get(), CheckCryptoSessionStatus(_, _))
      .WillOnce(DoAll(SetArgPointee<1>(D3D11_CRYPTO_SESSION_STATUS_OK),
                      Return(S_OK)));
  proxy_.CreateMediaCryptoSession(
      std::vector<uint8_t>(),
      base::BindOnce(&CallbackMock::CreateMediaCryptoSessionCallback,
                     base::Unretained(&callback_mock_)));
}

// |arg| is void*. This casts the pointer to uint8_t* and checks whether they
// match.
MATCHER_P(CastedToUint8Are, expected, "") {
  const uint8_t* actual = static_cast<const uint8_t*>(arg);
  for (size_t i = 0; i < expected.size(); ++i) {
    if (actual[i] != expected[i]) {
      *result_listener << "Mismatch at element " << i;
      return false;
    }
  }
  return true;
}

TEST_F(IntelCSMECdmProxyTest, CreateMediaCryptoSessionWithExtraData) {
  InjectMockD3D11CreateDeviceFunction();
  auto device_mock = CreateMock<D3D11DeviceMock>();
  auto video_device_mock = CreateMock<D3D11VideoDeviceMock>();
  auto video_device1_mock = CreateMock<D3D11VideoDevice1Mock>();
  auto crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
  auto device_context_mock = CreateMock<D3D11DeviceContextMock>();
  auto video_context_mock = CreateMock<D3D11VideoContextMock>();
  auto video_context1_mock = CreateMock<D3D11VideoContext1Mock>();

  uint32_t crypto_session_id_from_initialize = 0;
  EXPECT_CALL(
      callback_mock_,
      InitializeCallback(
          CdmProxy::Status::kOk,
          CdmProxy::Protocol::kIntelConvergedSecurityAndManageabilityEngine, _))
      .WillOnce(SaveArg<2>(&crypto_session_id_from_initialize));
  EXPECT_CALL(create_device_mock_,
              Create(_, D3D_DRIVER_TYPE_HARDWARE, _, _, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<7>(device_mock.Get()),
                      SetArgPointee<8>(D3D_FEATURE_LEVEL_11_1),
                      SetArgPointee<9>(device_context_mock.Get()),
                      Return(S_OK)));
  EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<1>(video_device_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*video_device_mock.Get(), CreateCryptoSession(_, _, _, _))
      .WillOnce(
          DoAll(SetArgPointee<3>(crypto_session_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_mock.Get(), QueryInterface(IID_ID3D11VideoDevice1, _))
      .Times(AtLeast(1))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_device1_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_context_mock.Get(),
              QueryInterface(IID_ID3D11VideoContext, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_context_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*device_context_mock.Get(),
              QueryInterface(IID_ID3D11VideoContext1, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(video_context1_mock.Get()), Return(S_OK)));

  EXPECT_CALL(*video_device1_mock.Get(),
              GetCryptoSessionPrivateDataSize(
                  Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _, _, _, _))
      .WillOnce(Return(S_OK));

  proxy_.Initialize(nullptr, base::BindOnce(&CallbackMock::InitializeCallback,
                                            base::Unretained(&callback_mock_)));
  ::testing::Mock::VerifyAndClearExpectations(&callback_mock_);

  // Expect a new crypto session.
  EXPECT_CALL(callback_mock_, CreateMediaCryptoSessionCallback(
                                  CdmProxy::Status::kOk,
                                  Ne(crypto_session_id_from_initialize), _));

  auto media_crypto_session_mock = CreateMock<D3D11CryptoSessionMock>();
  EXPECT_CALL(*video_device_mock.Get(),
              CreateCryptoSession(Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _,
                                  Pointee(D3D11_CONFIG_WIDEVINE_STREAM_ID), _))
      .WillOnce(DoAll(SetArgPointee<3>(media_crypto_session_mock.Get()),
                      Return(S_OK)));
  // The size nor value here matter, so making non empty non zero vector.
  const std::vector<uint8_t> kAnyInput(16, 0xFF);
  const uint64_t kAnyOutputData = 23298u;
  EXPECT_CALL(*video_context1_mock.Get(),
              GetDataForNewHardwareKey(media_crypto_session_mock.Get(),
                                       kAnyInput.size(),
                                       CastedToUint8Are(kAnyInput), _))
      .WillOnce(DoAll(SetArgPointee<3>(kAnyOutputData), Return(S_OK)));

  EXPECT_CALL(*video_context1_mock.Get(),
              CheckCryptoSessionStatus(media_crypto_session_mock.Get(), _))
      .WillOnce(DoAll(SetArgPointee<1>(D3D11_CRYPTO_SESSION_STATUS_OK),
                      Return(S_OK)));
  proxy_.CreateMediaCryptoSession(
      kAnyInput, base::BindOnce(&CallbackMock::CreateMediaCryptoSessionCallback,
                                base::Unretained(&callback_mock_)));
}

}  // namespace media