// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_proxy_service.h"

#include "base/bind.h"
#include "base/macros.h"
#include "media/mojo/services/mojo_cdm_service_context.h"

namespace media {

MojoCdmProxyService::MojoCdmProxyService(
    std::unique_ptr<::media::CdmProxy> cdm_proxy,
    MojoCdmServiceContext* context)
    : cdm_proxy_(std::move(cdm_proxy)), context_(context) {
  DVLOG(1) << __func__;
  DCHECK(cdm_proxy_);
  DCHECK(context_);
}

MojoCdmProxyService::~MojoCdmProxyService() {
  DVLOG(1) << __func__;
}

void MojoCdmProxyService::Initialize(
    mojom::CdmProxyClientAssociatedPtrInfo client,
    InitializeCallback callback) {
  DVLOG(2) << __func__;
  client_.Bind(std::move(client));

  // TODO(xhwang): Register |this| in the |context| to implement SetCdm().
  cdm_proxy_->Initialize(this, std::move(callback));
}

void MojoCdmProxyService::Process(media::CdmProxy::Function function,
                                  uint32_t crypto_session_id,
                                  const std::vector<uint8_t>& input_data,
                                  uint32_t expected_output_data_size,
                                  ProcessCallback callback) {
  DVLOG(3) << __func__;
  cdm_proxy_->Process(function, crypto_session_id, input_data,
                      expected_output_data_size, std::move(callback));
}

void MojoCdmProxyService::CreateMediaCryptoSession(
    const std::vector<uint8_t>& input_data,
    CreateMediaCryptoSessionCallback callback) {
  DVLOG(3) << __func__;
  cdm_proxy_->CreateMediaCryptoSession(input_data, std::move(callback));
}

void MojoCdmProxyService::SetKeyInfo(
    const std::vector<media::CdmProxyKeyInfo>& key_infos) {
  DVLOG(3) << __func__;
  // TODO(xhwang): Implement this once mojom::CdmProxy is updated.
  NOTIMPLEMENTED();
}

void MojoCdmProxyService::NotifyHardwareReset() {
  DVLOG(2) << __func__;
  client_->NotifyHardwareReset();
}

}  // namespace media
