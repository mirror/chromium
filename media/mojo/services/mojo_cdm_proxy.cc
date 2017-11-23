// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_proxy.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "media/base/scoped_callback_runner.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace media {

MojoCdmProxy::MojoCdmProxy(Delegate* delegate, mojom::CdmProxyPtr cdm_proxy_ptr)
    : delegate_(delegate),
      cdm_proxy_ptr_(std::move(cdm_proxy_ptr)),
      client_binding_(this),
      weak_factory_(this) {
  DVLOG(1) << __func__;
}

MojoCdmProxy::~MojoCdmProxy() {
  DVLOG(1) << __func__;
}

void MojoCdmProxy::Initialize(cdm::CdmProxyClient* client) {
  // DCHECK(client_);
  client_ = client;
  mojom::CdmProxyClientPtr client_ptr;
  client_binding_.Bind(mojo::MakeRequest(&client_ptr));

  auto callback = ScopedCallbackRunner(
      base::BindOnce(&MojoCdmProxy::OnInitialized, weak_factory_.GetWeakPtr()),
      media::CdmProxy::Status::kFail,
      media::CdmProxy::Protocol::kIntelConvergedSecurityAndManageabilityEngine,
      0);
  cdm_proxy_ptr_->Initialize(std::move(client_ptr), std::move(callback));
}

void MojoCdmProxy::Close() {
  DVLOG(3) << __func__;

  // Note: |this| could be deleted as part of this call.
  delegate_->CloseCdmProxy(this);
}

void MojoCdmProxy::NotifyHardwareReset() {
  DVLOG(3) << __func__;
}

void MojoCdmProxy::OnInitialized(media::CdmProxy::Status,
                                 media::CdmProxy::Protocol,
                                 uint32_t crypto_session_id) {
  DVLOG(3) << __func__;
  DVLOG(3) << __func__ << ": crypto_session_id = " << crypto_session_id;
  // DCHECK(client_);
  // client_->OnInitialized(crypto_session_id);
}

}  // namespace media
