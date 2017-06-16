// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_media_drm_storage.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"

namespace media {

MojoMediaDrmStorage::MojoMediaDrmStorage(
    mojom::MediaDrmStoragePtr media_drm_storage_ptr)
    : media_drm_storage_ptr_(std::move(media_drm_storage_ptr)),
      weak_factory_(this) {
  DVLOG(1) << __func__;
}

MojoMediaDrmStorage::~MojoMediaDrmStorage() {}

void MojoMediaDrmStorage::Initialize(const url::Origin& origin) {
  DVLOG(1) << __func__;

  // base::Unretained is safe because |this| owns |media_drm_storage_ptr_|.
  media_drm_storage_ptr_.set_connection_error_handler(base::Bind(
      &MojoMediaDrmStorage::OnConnectionError, base::Unretained(this)));

  media_drm_storage_ptr_->Initialize(origin);
}

void MojoMediaDrmStorage::OnProvisioned(ResultCB on_provisioned_cb) {
  DVLOG(1) << __func__;
  DCHECK(!on_provisioned_cb_);
  on_provisioned_cb_ = std::move(on_provisioned_cb);

  media_drm_storage_ptr_->OnProvisioned(
      base::Bind(&MojoMediaDrmStorage::OnResult, weak_factory_.GetWeakPtr()));
}

void MojoMediaDrmStorage::SavePersistentSession(
    const std::string& session_id,
    const SessionData& session_data,
    ResultCB save_persistent_session_cb) {
  DVLOG(1) << __func__;
  DCHECK(!save_persistent_session_cb_);
  save_persistent_session_cb_ = std::move(save_persistent_session_cb);

  media_drm_storage_ptr_->SavePersistentSession(
      session_id,
      mojom::SessionData::New(session_data.key_set_id, session_data.mime_type),
      base::Bind(&MojoMediaDrmStorage::OnResult, weak_factory_.GetWeakPtr()));
}

void MojoMediaDrmStorage::LoadPersistentSession(
    const std::string& session_id,
    LoadPersistentSessionCB load_persistent_session_cb) {
  DVLOG(1) << __func__;
  DCHECK(!load_persistent_session_cb_);
  load_persistent_session_cb_ = std::move(load_persistent_session_cb);

  media_drm_storage_ptr_->LoadPersistentSession(
      session_id, base::Bind(&MojoMediaDrmStorage::OnPersistentSessionLoaded,
                             weak_factory_.GetWeakPtr()));
}

void MojoMediaDrmStorage::RemovePersistentSession(
    const std::string& session_id,
    ResultCB remove_persistent_session_cb) {
  DVLOG(1) << __func__;
  DCHECK(!remove_persistent_session_cb_);
  remove_persistent_session_cb_ = std::move(remove_persistent_session_cb);

  media_drm_storage_ptr_->RemovePersistentSession(
      session_id,
      base::Bind(&MojoMediaDrmStorage::OnResult, weak_factory_.GetWeakPtr()));
}

void MojoMediaDrmStorage::OnResult(bool success) {
  DVLOG(1) << __func__ << ": success = " << success;
  if (on_provisioned_cb_)
    std::move(on_provisioned_cb_).Run(success);

  if (save_persistent_session_cb_)
    std::move(save_persistent_session_cb_).Run(success);

  if (remove_persistent_session_cb_)
    std::move(remove_persistent_session_cb_).Run(success);
}

void MojoMediaDrmStorage::OnPersistentSessionLoaded(
    mojom::SessionDataPtr session_data) {
  DVLOG(1) << __func__ << ": success = " << !!session_data;

  if (load_persistent_session_cb_) {
    std::move(load_persistent_session_cb_)
        .Run(session_data ? base::MakeUnique<SessionData>(
                                std::move(session_data->key_set_id),
                                std::move(session_data->mime_type))
                          : nullptr);
  }
}

void MojoMediaDrmStorage::OnConnectionError() {
  DVLOG(1) << __func__;

  if (on_provisioned_cb_)
    std::move(on_provisioned_cb_).Run(false);

  if (save_persistent_session_cb_)
    std::move(save_persistent_session_cb_).Run(false);

  if (remove_persistent_session_cb_)
    std::move(remove_persistent_session_cb_).Run(false);

  if (load_persistent_session_cb_)
    std::move(load_persistent_session_cb_).Run(nullptr);
}

}  // namespace media
