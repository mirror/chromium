// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/fake_sync_engine.h"

#include "components/sync/engine/activation_context.h"
#include "components/sync/engine/sync_engine_host.h"

namespace syncer {

const char kTestCacheGuid[] = "test-guid";

FakeSyncEngine::FakeSyncEngine() : fail_initial_download_(false) {}
FakeSyncEngine::~FakeSyncEngine() {}

void FakeSyncEngine::Initialize(
    SyncEngineHost* host,
    scoped_refptr<base::SingleThreadTaskRunner> sync_task_runner,
    const WeakHandle<JsEventHandler>& event_handler,
    const GURL& service_url,
    const std::string& sync_user_agent,
    const SyncCredentials& credentials,
    bool delete_sync_data_folder,
    bool enable_local_sync_backend,
    const base::FilePath& local_sync_backend_folder,
    std::unique_ptr<SyncManagerFactory> sync_manager_factory,
    const WeakHandle<UnrecoverableErrorHandler>& unrecoverable_error_handler,
    const base::Closure& report_unrecoverable_error_function,
    const HttpPostProviderFactoryGetter& http_post_provider_factory_getter,
    std::unique_ptr<SyncEncryptionHandler::NigoriState> saved_nigori_state) {
  host->OnEngineInitialized(ModelTypeSet(), WeakHandle<JsBackend>(),
                            WeakHandle<DataTypeDebugInfoListener>(),
                            kTestCacheGuid, !fail_initial_download_);
}

void FakeSyncEngine::TriggerRefresh(const ModelTypeSet& types) {}

void FakeSyncEngine::UpdateCredentials(const SyncCredentials& credentials) {}

void FakeSyncEngine::StartSyncingWithServer() {}

void FakeSyncEngine::SetEncryptionPassphrase(const std::string& passphrase,
                                             bool is_explicit) {}

bool FakeSyncEngine::SetDecryptionPassphrase(const std::string& passphrase) {
  return false;
}

void FakeSyncEngine::StopSyncingForShutdown() {}

void FakeSyncEngine::Shutdown(ShutdownReason reason) {}

void FakeSyncEngine::UnregisterInvalidationIds() {}

ModelTypeSet FakeSyncEngine::ConfigureDataTypes(
    ConfigureReason reason,
    const DataTypeConfigStateMap& config_state_map,
    const base::Callback<void(ModelTypeSet, ModelTypeSet)>& ready_task,
    const base::Callback<void()>& retry_callback) {
  return ModelTypeSet();
}

void FakeSyncEngine::EnableEncryptEverything() {}

void FakeSyncEngine::ActivateDirectoryDataType(
    ModelType type,
    ModelSafeGroup group,
    ChangeProcessor* change_processor) {}
void FakeSyncEngine::DeactivateDirectoryDataType(ModelType type) {}

void FakeSyncEngine::ActivateNonBlockingDataType(
    ModelType type,
    std::unique_ptr<ActivationContext> activation_context) {}

void FakeSyncEngine::DeactivateNonBlockingDataType(ModelType type) {}

UserShare* FakeSyncEngine::GetUserShare() const {
  return nullptr;
}

SyncEngine::Status FakeSyncEngine::GetDetailedStatus() {
  return SyncEngine::Status();
}

SyncCycleSnapshot FakeSyncEngine::GetLastCycleSnapshot() const {
  return SyncCycleSnapshot();
}

bool FakeSyncEngine::HasUnsyncedItems() const {
  return false;
}

bool FakeSyncEngine::IsNigoriEnabled() const {
  return true;
}

PassphraseType FakeSyncEngine::GetPassphraseType() const {
  return PassphraseType::IMPLICIT_PASSPHRASE;
}

base::Time FakeSyncEngine::GetExplicitPassphraseTime() const {
  return base::Time();
}

bool FakeSyncEngine::IsCryptographerReady(const BaseTransaction* trans) const {
  return false;
}

void FakeSyncEngine::GetModelSafeRoutingInfo(ModelSafeRoutingInfo* out) const {}

void FakeSyncEngine::FlushDirectory() const {}

void FakeSyncEngine::RefreshTypesForTest(ModelTypeSet types) {}

void FakeSyncEngine::RequestBufferedProtocolEventsAndEnableForwarding() {}

void FakeSyncEngine::DisableProtocolEventForwarding() {}

void FakeSyncEngine::EnableDirectoryTypeDebugInfoForwarding() {}

void FakeSyncEngine::DisableDirectoryTypeDebugInfoForwarding() {}

void FakeSyncEngine::set_fail_initial_download(bool should_fail) {
  fail_initial_download_ = should_fail;
}

void FakeSyncEngine::ClearServerData(
    const SyncManager::ClearServerDataCallback& callback) {
  callback.Run();
}

void FakeSyncEngine::OnCookieJarChanged(bool account_mismatch, bool empty_jar) {
}

}  // namespace syncer
