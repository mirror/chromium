// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_CDM_PROXY_H_
#define MEDIA_MOJO_SERVICES_MOJO_CDM_PROXY_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "media/cdm/api/content_decryption_module.h"
#include "media/mojo/interfaces/cdm_proxy.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

// Implements a cdm::CdmProxy that communicates with mojom::CdmProxy.
class MEDIA_MOJO_EXPORT MojoCdmProxy : public cdm::CdmProxy,
                                       mojom::CdmProxyClient {
 public:
  class Delegate {
   public:
    // Notifies the delegate to close |cdm_proxy|.
    virtual void CloseCdmProxy(MojoCdmProxy* cdm_proxy) = 0;
  };

  MojoCdmProxy(Delegate* delegate, mojom::CdmProxyPtr cdm_proxy_ptr);
  ~MojoCdmProxy() override;

  // cdm::CdmProxy implementation.
  void Initialize(cdm::CdmProxyClient* client) final;
  void Close() final;
  /*
  void Process(Function function,
                       uint32_t crypto_session_id,
                       const std::vector<uint8_t>& input_data,
                       uint32_t expected_output_data_size,
                       ) final;
  void CreateMediaCryptoSession(
      const std::vector<uint8_t>& input_data,
      CreateMediaCryptoSessionCB create_media_crypto_session_cb) final;

  // Send multiple key information to the proxy.
  void SetKeyInfo(const std::vector<CdmProxyKeyInfo>& key_infos) final;
  */

  // mojom::CdmProxyClient implementation.
  void NotifyHardwareReset() final;

 private:
  void OnInitialized(media::CdmProxy::Status,
                     media::CdmProxy::Protocol,
                     uint32_t crypto_session_id);

  Delegate* const delegate_;
  mojom::CdmProxyPtr cdm_proxy_ptr_;
  cdm::CdmProxyClient* client_;

  mojo::Binding<mojom::CdmProxyClient> client_binding_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MojoCdmProxy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoCdmProxy);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_CDM_PROXY_H_
