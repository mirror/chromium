// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_INTEL_CSME_CDM_PROXY_H_
#define MEDIA_GPU_INTEL_CSME_CDM_PROXY_H_

#include "media/cdm/cdm_proxy.h"

#include <d3d11_1.h>
#include <wrl/client.h>

#include <map>
#include <vector>

#include "base/callback.h"
#include "media/gpu/media_gpu_export.h"

namespace media {

// This is a CdmProxy implementation that uses Intel Converged Security and
// Manageability Engine.
class MEDIA_GPU_EXPORT IntelCSMECdmProxy : public CdmProxy {
 public:
  IntelCSMECdmProxy();
  ~IntelCSMECdmProxy() override;

  // CdmProxy implementation.
  void Initialize(Client* client, InitializeCB init_cb) override;
  void Process(Function function,
               uint32_t crypto_session_id,
               const std::vector<uint8_t>& input_data,
               uint32_t expected_output_data_size,
               ProcessCB process_cb) override;
  void CreateMediaCryptoSession(
      const std::vector<uint8_t>& input_data,
      CreateMediaCryptoSessionCB create_media_crypto_session_cb) override;
  void SetKeyInfo(const std::vector<KeyInfo>& key_infos) override;

 private:
  friend class IntelCSMECdmProxyTest;

  // Counter for assigning IDs to crypto sessions.
  uint32_t next_crypto_session_id_ = 1;

  Client* client_ = nullptr;
  bool initialized_ = false;

  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context_;
  // TODO(crbug.com/788880): Remove the ID3D11VideoDevice and ID3D11VideoContext
  // if unnecessary.
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> video_device_;
  Microsoft::WRL::ComPtr<ID3D11VideoDevice1> video_device1_;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> video_context_;
  Microsoft::WRL::ComPtr<ID3D11VideoContext1> video_context1_;

  std::map<uint32_t, Microsoft::WRL::ComPtr<ID3D11CryptoSession>>
      crypto_session_map_;
  // This should be updated with next_crypto_session_id_ when a CSME crypto
  // session is created.
  uint32_t csme_crypto_session_id_ = 0;

  // The values output from ID3D11VideoDevice1::GetCryptoSessionPrivateDataSize.
  // Used when calling NegotiateCryptoSessionKeyExchange.
  UINT private_input_size_ = 0;
  UINT private_output_size_ = 0;

  // Maps key ID to KeyInfo.
  std::map<std::vector<uint8_t>, KeyInfo> key_info_map_;

  // This create function is a member so that the implementation can be tests.
  // The test will swap this to return mock devices.
  // The template parameter matches the signature of D3D11CreateDevice().
  base::RepeatingCallback<HRESULT(IDXGIAdapter* adapter,
                                  D3D_DRIVER_TYPE driver_type,
                                  HMODULE software,
                                  UINT flags,
                                  const D3D_FEATURE_LEVEL* feature_levels,
                                  UINT feature_level,
                                  UINT sdk_version,
                                  ID3D11Device** device_out,
                                  D3D_FEATURE_LEVEL* feature_level_out,
                                  ID3D11DeviceContext** immediate_context_out)>
      create_device_func_;

  DISALLOW_COPY_AND_ASSIGN(IntelCSMECdmProxy);
};

}  // namespace media

#endif  // MEDIA_GPU_INTEL_CSME_CDM_PROXY_H_