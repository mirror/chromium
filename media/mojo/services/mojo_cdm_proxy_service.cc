// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_proxy_service.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "media/base/cdm_context.h"
#include "media/cdm/cdm_manager.h"
#include "media/mojo/services/mojo_cdm_service_context.h"

namespace media {

MojoCdmProxyService::MojoCdmProxyService(MojoCdmServiceContext* context)
    : context_(context), weak_factory_(this) {
  DCHECK(context_);
}

MojoCdmProxyService::~MojoCdmProxyService() {}

void MojoCdmProxyService::Initialize(mojom::CdmProxyClientPtr client,
                                     InitializeCallback callback) {
  DVLOG(2) << __func__;
  client_ = std::move(client);
  std::move(callback).Run(
      media::CdmProxy::Status::kOk,
      media::CdmProxy::Protocol::kIntelConvergedSecurityAndManageabilityEngine,
      100);
  // client_->NotifyHardwareReset();
}

void MojoCdmProxyService::Process(media::CdmProxy::Function function,
                                  uint32_t crypto_session_id,
                                  const std::vector<uint8_t>& input_data,
                                  uint32_t expected_output_data_size,
                                  ProcessCallback callback) {
  DVLOG(2) << __func__;
}

void MojoCdmProxyService::CreateMediaCryptoSession(
    const std::vector<uint8_t>& input_data,
    CreateMediaCryptoSessionCallback callback) {
  DVLOG(2) << __func__;
}

void MojoCdmProxyService::SetKeyInfo(
    const std::vector<media::CdmProxyKeyInfo>& key_infos) {
  DVLOG(2) << __func__;
}

}  // namespace media
