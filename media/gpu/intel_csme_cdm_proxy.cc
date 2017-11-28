// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(crbug.com/787657): Handle hardware key reset and notify the client.
#include "media/gpu/intel_csme_cdm_proxy.h"

#include <initguid.h>

#include "base/bind.h"
#include "base/logging.h"

namespace media {

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

IntelCSMECdmProxy::IntelCSMECdmProxy()
    : create_device_func_(base::BindRepeating(D3D11CreateDevice)) {}
IntelCSMECdmProxy::~IntelCSMECdmProxy() {}

void IntelCSMECdmProxy::Initialize(Client* client, InitializeCB init_cb) {
  if (initialized_) {
    std::move(init_cb).Run(
        Status::kOk, Protocol::kIntelConvergedSecurityAndManageabilityEngine,
        csme_crypto_session_id_);
    return;
  }

  auto failed = [this, &init_cb]() {
    std::move(init_cb).Run(
        Status::kFail, Protocol::kIntelConvergedSecurityAndManageabilityEngine,
        csme_crypto_session_id_);
  };

  client_ = client;

  D3D_FEATURE_LEVEL actual_feature_level = {};
  D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1,
                                        D3D_FEATURE_LEVEL_11_0};
  HRESULT hresult = create_device_func_.Run(
      nullptr,                            // No adapter.
      D3D_DRIVER_TYPE_HARDWARE, nullptr,  // No software rasterizer.
      0,                                  // flags, none.
      feature_levels, arraysize(feature_levels), D3D11_SDK_VERSION,
      device_.GetAddressOf(), &actual_feature_level,
      device_context_.GetAddressOf());
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to create the D3D11Device, status:" << hresult;
    failed();
    return;
  }
  if (actual_feature_level != D3D_FEATURE_LEVEL_11_1) {
    LOG(ERROR) << "D3D11 feature level too low: " << actual_feature_level;
    failed();
    return;
  }

  hresult = device_.CopyTo(video_device_.GetAddressOf());
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to get ID3D11VideoDevice.";
    failed();
    return;
  }

  hresult = device_context_.CopyTo(video_context_.GetAddressOf());
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to get ID3D11VideoContext, status: " << hresult;
    failed();
    return;
  }

  hresult = device_.CopyTo(video_device1_.GetAddressOf());
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to get ID3D11VideoDevice1, status: " << hresult;
    failed();
    return;
  }

  hresult = device_context_.CopyTo(video_context1_.GetAddressOf());
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to get ID3D11VideoContext1, status: " << hresult;
    failed();
    return;
  }

  Microsoft::WRL::ComPtr<ID3D11CryptoSession> csme_crypto_session;
  hresult = video_device_->CreateCryptoSession(
      &D3D11_CONFIG_WIDEVINE_STREAM_ID, &D3D11_DECODER_PROFILE_H264_VLD_NOFGT,
      &D3D11_KEY_EXCHANGE_HW_PROTECTION, csme_crypto_session.GetAddressOf());
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to Create CryptoSession, status: " << hresult;
    failed();
    return;
  }

  hresult = video_device1_->GetCryptoSessionPrivateDataSize(
      &D3D11_CONFIG_WIDEVINE_STREAM_ID, &D3D11_DECODER_PROFILE_H264_VLD_NOFGT,
      &D3D11_KEY_EXCHANGE_HW_PROTECTION, &private_input_size_,
      &private_output_size_);
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to get private data sizes, status" << hresult;
    failed();
    return;
  }

  csme_crypto_session_id_ = next_crypto_session_id_++;
  crypto_session_map_[csme_crypto_session_id_] = std::move(csme_crypto_session);
  initialized_ = true;
  std::move(init_cb).Run(
      Status::kOk, Protocol::kIntelConvergedSecurityAndManageabilityEngine,
      csme_crypto_session_id_);
}

void IntelCSMECdmProxy::Process(Function function,
                                uint32_t crypto_session_id,
                                const std::vector<uint8_t>& input_data_vec,
                                uint32_t expected_output_data_size,
                                ProcessCB process_cb) {
  auto failed = [&process_cb]() {
    std::move(process_cb).Run(Status::kFail, std::vector<uint8_t>());
  };
  if (!initialized_ ||
      function != Function::kIntelNegotiateCryptoSessionKeyExchange) {
    LOG_IF(ERROR, !initialized_) << "Not initialied.";
    LOG_IF(ERROR, function != Function::kIntelNegotiateCryptoSessionKeyExchange)
        << "Unrecognized function: " << static_cast<int>(function);
    failed();
    return;
  }

  if (crypto_session_map_.find(crypto_session_id) ==
      crypto_session_map_.end()) {
    LOG(ERROR) << "Cannot find crypto session with ID " << crypto_session_id;
    failed();
    return;
  }

  Microsoft::WRL::ComPtr<ID3D11CryptoSession>& crypto_session =
      crypto_session_map_[crypto_session_id];

  D3D11_KEY_EXCHANGE_HW_PROTECTION_DATA key_exchange_data = {};
  key_exchange_data.HWProtectionFunctionID = 0x90000001;

  // Because D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA and
  // D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA are variable size structures,
  // uint8 array are allocated and casted to each type.
  // -4 for the "BYTE pbInput[4]" field.
  std::unique_ptr<uint8_t[]> input_data_raw(
      new uint8_t[sizeof(D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA) - 4 +
                  input_data_vec.size()]);
  std::unique_ptr<uint8_t[]> output_data_raw(
      new uint8_t[sizeof(D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA) - 4 +
                  expected_output_data_size]);

  D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA* input_data =
      reinterpret_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_INPUT_DATA*>(
          input_data_raw.get());
  D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA* output_data =
      reinterpret_cast<D3D11_KEY_EXCHANGE_HW_PROTECTION_OUTPUT_DATA*>(
          output_data_raw.get());

  key_exchange_data.pInputData = input_data;
  key_exchange_data.pOutputData = output_data;
  input_data->PrivateDataSize = private_input_size_;
  input_data->HWProtectionDataSize = 0;
  memcpy(input_data->pbInput, input_data_vec.data(), input_data_vec.size());

  output_data->PrivateDataSize = private_output_size_;
  output_data->HWProtectionDataSize = 0;
  output_data->TransportTime = 0;
  output_data->ExecutionTime = 0;
  output_data->MaxHWProtectionDataSize = expected_output_data_size;

  HRESULT hresult = video_context_->NegotiateCryptoSessionKeyExchange(
      crypto_session.Get(), sizeof(key_exchange_data), &key_exchange_data);
  if (FAILED(hresult)) {
    failed();
    return;
  }

  std::vector<uint8_t> output_vec;
  output_vec.reserve(expected_output_data_size);
  memcpy(output_vec.data(), output_data->pbOutput, expected_output_data_size);
  std::move(process_cb).Run(Status::kOk, output_vec);
  return;
}

void IntelCSMECdmProxy::CreateMediaCryptoSession(
    const std::vector<uint8_t>& input_data,
    CreateMediaCryptoSessionCB create_media_crypto_session_cb) {
  auto failed = [&create_media_crypto_session_cb]() {
    const uint32_t kInvalidSessionId = 0;
    const uint64_t kNoOutputData = 0;
    std::move(create_media_crypto_session_cb)
        .Run(Status::kFail, kInvalidSessionId, kNoOutputData);
  };
  if (!initialized_) {
    LOG(ERROR) << "Not initialized.";
    failed();
  }

  Microsoft::WRL::ComPtr<ID3D11CryptoSession> media_crypto_session;
  HRESULT hresult = video_device_->CreateCryptoSession(
      &D3D11_CONFIG_WIDEVINE_STREAM_ID, &D3D11_DECODER_PROFILE_H264_VLD_NOFGT,
      &D3D11_CONFIG_WIDEVINE_STREAM_ID, media_crypto_session.GetAddressOf());
  if (FAILED(hresult)) {
    LOG(ERROR) << "Failed to create a crypto session.";
    failed();
    return;
  }

  // Don't do CheckCryptoSessionStatus() yet. The status may be something like
  // CONTEXT_LOST because GetDataForNewHardwareKey() is not called yet.
  uint64_t output_data = 0;
  if (!input_data.empty()) {
    hresult = video_context1_->GetDataForNewHardwareKey(
        media_crypto_session.Get(), input_data.size(), input_data.data(),
        &output_data);
    if (FAILED(hresult)) {
      LOG(ERROR) << "Failed to establish hardware session.";
      failed();
      return;
    }
  }

  D3D11_CRYPTO_SESSION_STATUS crypto_session_status = {};
  hresult = video_context1_->CheckCryptoSessionStatus(
      media_crypto_session.Get(), &crypto_session_status);
  if (FAILED(hresult) ||
      crypto_session_status != D3D11_CRYPTO_SESSION_STATUS_OK) {
    LOG(ERROR) << "Crypto session is not OK. Crypto session status "
               << crypto_session_status << ". HRESULT " << hresult;
    failed();
    return;
  }

  const uint32_t media_crypto_session_id = next_crypto_session_id_++;
  crypto_session_map_[media_crypto_session_id] =
      std::move(media_crypto_session);
  std::move(create_media_crypto_session_cb)
      .Run(Status::kOk, media_crypto_session_id, output_data);
}

void IntelCSMECdmProxy::SetKeyInfo(const std::vector<KeyInfo>& key_infos) {
  for (const KeyInfo& key_info : key_infos) {
    if (key_info.is_usable_key) {
      key_info_map_[key_info.key_id] = key_info;
    } else {
      key_info_map_.erase(key_info.key_id);
    }
  }
}

}  // namespace media