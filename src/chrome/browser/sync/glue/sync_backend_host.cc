// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#include <algorithm>

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/file_util.h"
#include "base/task.h"
#include "base/threading/thread_restrictions.h"
#include "base/tracked.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/net/gaia/token_service.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/engine/syncapi.h"
#include "chrome/browser/sync/glue/autofill_model_associator.h"
#include "chrome/browser/sync/glue/autofill_profile_model_associator.h"
#include "chrome/browser/sync/glue/change_processor.h"
#include "chrome/browser/sync/glue/database_model_worker.h"
#include "chrome/browser/sync/glue/history_model_worker.h"
#include "chrome/browser/sync/glue/http_bridge.h"
#include "chrome/browser/sync/glue/password_model_worker.h"
#include "chrome/browser/sync/glue/sync_backend_host.h"
#include "chrome/browser/sync/js_arg_list.h"
#include "chrome/browser/sync/js_event_details.h"
#include "chrome/browser/sync/notifier/sync_notifier.h"
#include "chrome/browser/sync/sessions/session_state.h"
// TODO(tim): Remove this! We should have a syncapi pass-thru instead.
#include "chrome/browser/sync/syncable/directory_manager.h"  // Cryptographer.
#include "chrome/browser/sync/syncable/model_type.h"
#include "chrome/browser/sync/syncable/nigori_util.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "chrome/common/pref_names.h"
#include "content/browser/browser_thread.h"
#include "content/common/notification_service.h"
#include "googleurl/src/gurl.h"
#include "webkit/glue/webkit_glue.h"

static const int kSaveChangesIntervalSeconds = 10;
static const FilePath::CharType kSyncDataFolderName[] =
    FILE_PATH_LITERAL("Sync Data");

using browser_sync::DataTypeController;
typedef TokenService::TokenAvailableDetails TokenAvailableDetails;

typedef GoogleServiceAuthError AuthError;

namespace browser_sync {

using sessions::SyncSessionSnapshot;
using sync_api::SyncCredentials;

SyncBackendHost::SyncBackendHost(Profile* profile)
    : core_(new Core(profile->GetDebugName(),
                     ALLOW_THIS_IN_INITIALIZER_LIST(this))),
      initialization_state_(NOT_INITIALIZED),
      sync_thread_("Chrome_SyncThread"),
      frontend_loop_(MessageLoop::current()),
      profile_(profile),
      sync_notifier_factory_(webkit_glue::GetUserAgent(GURL()),
                             profile_->GetRequestContext(),
                             *CommandLine::ForCurrentProcess()),
      frontend_(NULL),
      sync_data_folder_path_(
          profile_->GetPath().Append(kSyncDataFolderName)),
      last_auth_error_(AuthError::None()) {
}

SyncBackendHost::SyncBackendHost()
    : initialization_state_(NOT_INITIALIZED),
      sync_thread_("Chrome_SyncThread"),
      frontend_loop_(MessageLoop::current()),
      profile_(NULL),
      sync_notifier_factory_(webkit_glue::GetUserAgent(GURL()),
                             NULL,
                             *CommandLine::ForCurrentProcess()),
      frontend_(NULL),
      last_auth_error_(AuthError::None()) {
}

SyncBackendHost::~SyncBackendHost() {
  DCHECK(!core_ && !frontend_) << "Must call Shutdown before destructor.";
  DCHECK(registrar_.workers.empty());
}

void SyncBackendHost::Initialize(
    SyncFrontend* frontend,
    const GURL& sync_service_url,
    const syncable::ModelTypeSet& types,
    const SyncCredentials& credentials,
    bool delete_sync_data_folder) {
  if (!sync_thread_.Start())
    return;

  frontend_ = frontend;
  DCHECK(frontend);

  registrar_.workers[GROUP_DB] = new DatabaseModelWorker();
  registrar_.workers[GROUP_UI] = new UIModelWorker();
  registrar_.workers[GROUP_PASSIVE] = new ModelSafeWorker();
  registrar_.workers[GROUP_HISTORY] = new HistoryModelWorker(
      profile_->GetHistoryService(Profile::IMPLICIT_ACCESS));

  // Any datatypes that we want the syncer to pull down must
  // be in the routing_info map.  We set them to group passive, meaning that
  // updates will be applied to sync, but not dispatched to the native models.
  for (syncable::ModelTypeSet::const_iterator it = types.begin();
      it != types.end(); ++it) {
    registrar_.routing_info[(*it)] = GROUP_PASSIVE;
  }

  if (profile_->GetPrefs()->GetBoolean(prefs::kSyncHasSetupCompleted))
    registrar_.routing_info[syncable::NIGORI] = GROUP_PASSIVE;

  PasswordStore* password_store =
      profile_->GetPasswordStore(Profile::IMPLICIT_ACCESS);
  if (password_store) {
    registrar_.workers[GROUP_PASSWORD] =
        new PasswordModelWorker(password_store);
  } else {
    LOG_IF(WARNING, types.count(syncable::PASSWORDS) > 0) << "Password store "
        << "not initialized, cannot sync passwords";
    registrar_.routing_info.erase(syncable::PASSWORDS);
  }

  InitCore(Core::DoInitializeOptions(
      sync_service_url,
      profile_->GetRequestContext(),
      credentials,
      delete_sync_data_folder,
      RestoreEncryptionBootstrapToken(),
      false));
}

void SyncBackendHost::PersistEncryptionBootstrapToken(
    const std::string& token) {
  PrefService* prefs = profile_->GetPrefs();

  prefs->SetString(prefs::kEncryptionBootstrapToken, token);
  prefs->ScheduleSavePersistentPrefs();
}

std::string SyncBackendHost::RestoreEncryptionBootstrapToken() {
  PrefService* prefs = profile_->GetPrefs();
  std::string token = prefs->GetString(prefs::kEncryptionBootstrapToken);
  return token;
}

bool SyncBackendHost::IsNigoriEnabled() const {
  base::AutoLock lock(registrar_lock_);
  return registrar_.routing_info.find(syncable::NIGORI) !=
      registrar_.routing_info.end();
}

bool SyncBackendHost::IsUsingExplicitPassphrase() {
  return IsNigoriEnabled() && initialized() &&
      core_->sync_manager()->InitialSyncEndedForAllEnabledTypes() &&
      core_->sync_manager()->IsUsingExplicitPassphrase();
}

bool SyncBackendHost::IsCryptographerReady(
    const sync_api::BaseTransaction* trans) const {
  return initialized() && trans->GetCryptographer()->is_ready();
}

JsBackend* SyncBackendHost::GetJsBackend() {
  if (initialized()) {
    return core_.get();
  } else {
    NOTREACHED();
    return NULL;
  }
}

sync_api::HttpPostProviderFactory* SyncBackendHost::MakeHttpBridgeFactory(
    const scoped_refptr<net::URLRequestContextGetter>& getter) {
  return new HttpBridgeFactory(getter);
}

void SyncBackendHost::InitCore(const Core::DoInitializeOptions& options) {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(core_.get(), &SyncBackendHost::Core::DoInitialize,
                        options));
}

void SyncBackendHost::UpdateCredentials(const SyncCredentials& credentials) {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(core_.get(),
                        &SyncBackendHost::Core::DoUpdateCredentials,
                        credentials));
}

void SyncBackendHost::StartSyncingWithServer() {
  VLOG(1) << "SyncBackendHost::StartSyncingWithServer called.";
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(core_.get(), &SyncBackendHost::Core::DoStartSyncing));
}

void SyncBackendHost::SetPassphrase(const std::string& passphrase,
                                    bool is_explicit) {
  if (!IsNigoriEnabled()) {
    LOG(WARNING) << "Silently dropping SetPassphrase request.";
    return;
  }

  // This should only be called by the frontend.
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  if (core_->processing_passphrase()) {
    VLOG(1) << "Attempted to call SetPassphrase while already waiting for "
            << " result from previous SetPassphrase call. Silently dropping.";
    return;
  }
  core_->set_processing_passphrase();

  // If encryption is enabled and we've got a SetPassphrase
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(core_.get(), &SyncBackendHost::Core::DoSetPassphrase,
                        passphrase, is_explicit));
}

void SyncBackendHost::Shutdown(bool sync_disabled) {
  // Thread shutdown should occur in the following order:
  // - Sync Thread
  // - UI Thread (stops some time after we return from this call).
  if (sync_thread_.IsRunning()) {  // Not running in tests.
    // TODO(akalin): Remove the need for this.
    if (initialization_state_ > NOT_INITIALIZED) {
      core_->sync_manager()->RequestEarlyExit();
    }
    sync_thread_.message_loop()->PostTask(FROM_HERE,
        NewRunnableMethod(core_.get(),
                          &SyncBackendHost::Core::DoShutdown,
                          sync_disabled));
  }

  // Before joining the sync_thread_, we wait for the UIModelWorker to
  // give us the green light that it is not depending on the frontend_loop_ to
  // process any more tasks. Stop() blocks until this termination condition
  // is true.
  if (ui_worker())
    ui_worker()->Stop();

  // Stop will return once the thread exits, which will be after DoShutdown
  // runs. DoShutdown needs to run from sync_thread_ because the sync backend
  // requires any thread that opened sqlite handles to relinquish them
  // personally. We need to join threads, because otherwise the main Chrome
  // thread (ui loop) can exit before DoShutdown finishes, at which point
  // virtually anything the sync backend does (or the post-back to
  // frontend_loop_ by our Core) will epically fail because the CRT won't be
  // initialized.
  // Since we are blocking the UI thread here, we need to turn ourselves in
  // with the ThreadRestriction police.  For sentencing and how we plan to fix
  // this, see bug 19757.
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    sync_thread_.Stop();
  }

  registrar_.routing_info.clear();
  registrar_.workers[GROUP_DB] = NULL;
  registrar_.workers[GROUP_HISTORY] = NULL;
  registrar_.workers[GROUP_UI] = NULL;
  registrar_.workers[GROUP_PASSIVE] = NULL;
  registrar_.workers[GROUP_PASSWORD] = NULL;
  registrar_.workers.erase(GROUP_DB);
  registrar_.workers.erase(GROUP_HISTORY);
  registrar_.workers.erase(GROUP_UI);
  registrar_.workers.erase(GROUP_PASSIVE);
  registrar_.workers.erase(GROUP_PASSWORD);
  frontend_ = NULL;
  core_ = NULL;  // Releases reference to core_.
}

SyncBackendHost::PendingConfigureDataTypesState::
PendingConfigureDataTypesState() : deleted_type(false),
    reason(sync_api::CONFIGURE_REASON_UNKNOWN) {}

SyncBackendHost::PendingConfigureDataTypesState::
~PendingConfigureDataTypesState() {}

SyncBackendHost::PendingConfigureDataTypesState*
    SyncBackendHost::MakePendingConfigModeState(
        const DataTypeController::TypeMap& data_type_controllers,
        const syncable::ModelTypeSet& types,
        CancelableTask* ready_task,
        ModelSafeRoutingInfo* routing_info,
        sync_api::ConfigureReason reason,
        bool nigori_enabled) {
  PendingConfigureDataTypesState* state = new PendingConfigureDataTypesState();
  for (DataTypeController::TypeMap::const_iterator it =
           data_type_controllers.begin();
       it != data_type_controllers.end(); ++it) {
    syncable::ModelType type = it->first;
    // If a type is not specified, remove it from the routing_info.
    if (types.count(type) == 0) {
      state->deleted_type = true;
      routing_info->erase(type);
    } else {
      // Add a newly specified data type as GROUP_PASSIVE into the
      // routing_info, if it does not already exist.
      if (routing_info->count(type) == 0) {
        (*routing_info)[type] = GROUP_PASSIVE;
        state->added_types.set(type);
      }
    }
  }

  // We must handle NIGORI separately as it has no DataTypeController.
  if (types.count(syncable::NIGORI) == 0) {
    if (nigori_enabled) { // Nigori is currently enabled.
      state->deleted_type = true;
      routing_info->erase(syncable::NIGORI);
      // IsNigoriEnabled is now false.
    }
  } else { // Nigori needs to be enabled.
    if (!nigori_enabled) {
      // Currently it is disabled. So enable it.
      (*routing_info)[syncable::NIGORI] = GROUP_PASSIVE;
      state->added_types.set(syncable::NIGORI);
    }
  }

  state->ready_task.reset(ready_task);
  state->initial_types = types;
  state->reason = reason;
  return state;
}

void SyncBackendHost::ConfigureDataTypes(
    const DataTypeController::TypeMap& data_type_controllers,
    const syncable::ModelTypeSet& types,
    sync_api::ConfigureReason reason,
    CancelableTask* ready_task,
    bool enable_nigori) {
  // Only one configure is allowed at a time.
  DCHECK(!pending_config_mode_state_.get());
  DCHECK(!pending_download_state_.get());
  DCHECK_GT(initialization_state_, NOT_INITIALIZED);

  syncable::ModelTypeSet types_copy = types;
  if (enable_nigori) {
    types_copy.insert(syncable::NIGORI);
  }
  bool nigori_currently_enabled = IsNigoriEnabled();
  {
    base::AutoLock lock(registrar_lock_);
    pending_config_mode_state_.reset(
        MakePendingConfigModeState(data_type_controllers, types_copy,
                                   ready_task, &registrar_.routing_info, reason,
                                   nigori_currently_enabled));
  }

  StartConfiguration(NewCallback(core_.get(),
      &SyncBackendHost::Core::FinishConfigureDataTypes));
}

void SyncBackendHost::StartConfiguration(Callback0::Type* callback) {
  // Put syncer in the config mode. DTM will put us in normal mode once it is
  // done. This is to ensure we dont do a normal sync when we are doing model
  // association.
  sync_thread_.message_loop()->PostTask(FROM_HERE, NewRunnableMethod(
      core_.get(), &SyncBackendHost::Core::DoStartConfiguration, callback));
}

void SyncBackendHost::FinishConfigureDataTypesOnFrontendLoop() {
  DCHECK_EQ(MessageLoop::current(), frontend_loop_);
  // Nudge the syncer. This is necessary for both datatype addition/deletion.
  //
  // Deletions need a nudge in order to ensure the deletion occurs in a timely
  // manner (see issue 56416).
  //
  // In the case of additions, on the next sync cycle, the syncer should
  // notice that the routing info has changed and start the process of
  // downloading updates for newly added data types.  Once this is
  // complete, the configure_state_.ready_task_ is run via an
  // OnInitializationComplete notification.

  VLOG(1) << "Syncer in config mode. SBH executing"
          << "FinishConfigureDataTypesOnFrontendLoop";
  if (pending_config_mode_state_->deleted_type) {
    sync_thread_.message_loop()->PostTask(FROM_HERE,
        NewRunnableMethod(core_.get(),
        &SyncBackendHost::Core::DeferNudgeForCleanup));
  }

  if (pending_config_mode_state_->added_types.none() &&
      !core_->sync_manager()->InitialSyncEndedForAllEnabledTypes()) {
    LOG(WARNING) << "No new types, but initial sync not finished."
                 << "Possible sync db corruption / removal.";
    // TODO(tim): Log / UMA / count this somehow?
    // TODO(tim): If no added types, we could (should?) config only for
    // types that are needed... but this is a rare corruption edge case or
    // implies the user mucked around with their syncdb, so for now do all.
    pending_config_mode_state_->added_types =
        syncable::ModelTypeBitSetFromSet(
            pending_config_mode_state_->initial_types);
  }

  // If we've added types, we always want to request a nudge/config (even if
  // the initial sync is ended), in case we could not decrypt the data.
  if (pending_config_mode_state_->added_types.none()) {
    VLOG(1) << "SyncBackendHost(" << this << "): No new types added. "
            << "Calling ready_task directly";
    // No new types - just notify the caller that the types are available.
    pending_config_mode_state_->ready_task->Run();
  } else {
    pending_download_state_.reset(pending_config_mode_state_.release());

    syncable::ModelTypeBitSet types_copy(pending_download_state_->added_types);
    if (IsNigoriEnabled())
      types_copy.set(syncable::NIGORI);
    VLOG(1) <<  "SyncBackendHost(" << this << "):New Types added. "
            << "Calling DoRequestConfig";
    sync_thread_.message_loop()->PostTask(FROM_HERE,
         NewRunnableMethod(core_.get(),
                           &SyncBackendHost::Core::DoRequestConfig,
                           types_copy,
                           pending_download_state_->reason));
  }

  pending_config_mode_state_.reset();

  // Notify the SyncManager about the new types.
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(core_.get(),
                        &SyncBackendHost::Core::DoUpdateEnabledTypes));
}

void SyncBackendHost::EncryptDataTypes(
    const syncable::ModelTypeSet& encrypted_types) {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
     NewRunnableMethod(core_.get(),
                       &SyncBackendHost::Core::DoEncryptDataTypes,
                       encrypted_types));
}

syncable::ModelTypeSet SyncBackendHost::GetEncryptedDataTypes() const {
  DCHECK_GT(initialization_state_, NOT_INITIALIZED);
  return core_->sync_manager()->GetEncryptedDataTypes();
}

void SyncBackendHost::RequestNudge(const tracked_objects::Location& location) {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(core_.get(), &SyncBackendHost::Core::DoRequestNudge,
                        location));
}

void SyncBackendHost::ActivateDataType(
    DataTypeController* data_type_controller,
    ChangeProcessor* change_processor) {
  base::AutoLock lock(registrar_lock_);

  // Ensure that the given data type is in the PASSIVE group.
  browser_sync::ModelSafeRoutingInfo::iterator i =
      registrar_.routing_info.find(data_type_controller->type());
  DCHECK(i != registrar_.routing_info.end());
  DCHECK((*i).second == GROUP_PASSIVE);
  syncable::ModelType type = data_type_controller->type();
  // Change the data type's routing info to its group.
  registrar_.routing_info[type] = data_type_controller->model_safe_group();

  // Add the data type's change processor to the list of change
  // processors so it can receive updates.
  DCHECK_EQ(processors_.count(type), 0U);
  processors_[type] = change_processor;

  // Start the change processor.
  change_processor->Start(profile_, GetUserShare());
}

void SyncBackendHost::DeactivateDataType(
    DataTypeController* data_type_controller,
    ChangeProcessor* change_processor) {
  base::AutoLock lock(registrar_lock_);
  registrar_.routing_info.erase(data_type_controller->type());

  // Stop the change processor and remove it from the list of processors.
  change_processor->Stop();
  std::map<syncable::ModelType, ChangeProcessor*>::size_type erased =
      processors_.erase(data_type_controller->type());
  DCHECK_EQ(erased, 1U);
}

bool SyncBackendHost::RequestClearServerData() {
  sync_thread_.message_loop()->PostTask(FROM_HERE,
     NewRunnableMethod(core_.get(),
     &SyncBackendHost::Core::DoRequestClearServerData));
  return true;
}

SyncBackendHost::Core::~Core() {
  DCHECK(!sync_manager_.get());
}

void SyncBackendHost::Core::NotifyPassphraseRequired(
    sync_api::PassphraseRequiredReason reason) {
  if (!host_ || !host_->frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);

  // When setting a passphrase fails, unset our waiting flag.
  if (reason == sync_api::REASON_SET_PASSPHRASE_FAILED)
    processing_passphrase_ = false;

  if (processing_passphrase_) {
    VLOG(1) << "Core received OnPassphraseRequired while processing a "
            << "passphrase. Silently dropping.";
    return;
  }

  host_->frontend_->OnPassphraseRequired(reason);
}

void SyncBackendHost::Core::NotifyPassphraseAccepted(
    const std::string& bootstrap_token) {
  if (!host_ || !host_->frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);

  processing_passphrase_ = false;
  host_->PersistEncryptionBootstrapToken(bootstrap_token);
  host_->frontend_->OnPassphraseAccepted();
}

void SyncBackendHost::Core::NotifyUpdatedToken(const std::string& token) {
  if (!host_)
    return;
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TokenAvailableDetails details(GaiaConstants::kSyncService, token);
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_TOKEN_UPDATED,
      NotificationService::AllSources(),
      Details<const TokenAvailableDetails>(&details));
}

void SyncBackendHost::Core::NotifyEncryptionComplete(
    const syncable::ModelTypeSet& encrypted_types) {
  if (!host_)
    return;
  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);
  host_->frontend_->OnEncryptionComplete(encrypted_types);
}

void SyncBackendHost::Core::FinishConfigureDataTypes() {
  host_->frontend_loop_->PostTask(FROM_HERE, NewRunnableMethod(this,
      &SyncBackendHost::Core::FinishConfigureDataTypesOnFrontendLoop));
}

void SyncBackendHost::Core::FinishConfigureDataTypesOnFrontendLoop() {
  host_->FinishConfigureDataTypesOnFrontendLoop();
}


SyncBackendHost::Core::DoInitializeOptions::DoInitializeOptions(
    const GURL& service_url,
    const scoped_refptr<net::URLRequestContextGetter>&
        request_context_getter,
    const sync_api::SyncCredentials& credentials,
    bool delete_sync_data_folder,
    const std::string& restored_key_for_bootstrapping,
    bool setup_for_test_mode)
    : service_url(service_url),
      request_context_getter(request_context_getter),
      credentials(credentials),
      delete_sync_data_folder(delete_sync_data_folder),
      restored_key_for_bootstrapping(restored_key_for_bootstrapping),
      setup_for_test_mode(setup_for_test_mode) {
}

SyncBackendHost::Core::DoInitializeOptions::~DoInitializeOptions() {}

sync_api::UserShare* SyncBackendHost::GetUserShare() const {
  DCHECK(initialized());
  return core_->sync_manager()->GetUserShare();
}

SyncBackendHost::Status SyncBackendHost::GetDetailedStatus() {
  DCHECK(initialized());
  return core_->sync_manager()->GetDetailedStatus();
}

SyncBackendHost::StatusSummary SyncBackendHost::GetStatusSummary() {
  DCHECK(initialized());
  return core_->sync_manager()->GetStatusSummary();
}

string16 SyncBackendHost::GetAuthenticatedUsername() const {
  DCHECK(initialized());
  return UTF8ToUTF16(core_->sync_manager()->GetAuthenticatedUsername());
}

const GoogleServiceAuthError& SyncBackendHost::GetAuthError() const {
  return last_auth_error_;
}

const SyncSessionSnapshot* SyncBackendHost::GetLastSessionSnapshot() const {
  return last_snapshot_.get();
}

void SyncBackendHost::GetWorkers(std::vector<ModelSafeWorker*>* out) {
  base::AutoLock lock(registrar_lock_);
  out->clear();
  for (WorkerMap::const_iterator it = registrar_.workers.begin();
       it != registrar_.workers.end(); ++it) {
    out->push_back((*it).second);
  }
}

void SyncBackendHost::GetModelSafeRoutingInfo(ModelSafeRoutingInfo* out) {
  base::AutoLock lock(registrar_lock_);
  ModelSafeRoutingInfo copy(registrar_.routing_info);
  out->swap(copy);
}

bool SyncBackendHost::HasUnsyncedItems() const {
  DCHECK(initialized());
  return core_->sync_manager()->HasUnsyncedItems();
}

void SyncBackendHost::LogUnsyncedItems(int level) const {
  DCHECK(initialized());
  return core_->sync_manager()->LogUnsyncedItems(level);
}

SyncBackendHost::Core::Core(const std::string& name, SyncBackendHost* backend)
    : name_(name),
      host_(backend),
      sync_manager_observer_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
      parent_router_(NULL),
      processing_passphrase_(false),
      deferred_nudge_for_cleanup_requested_(false) {
}

// Helper to construct a user agent string (ASCII) suitable for use by
// the syncapi for any HTTP communication. This string is used by the sync
// backend for classifying client types when calculating statistics.
std::string MakeUserAgentForSyncApi() {
  std::string user_agent;
  user_agent = "Chrome ";
#if defined(OS_WIN)
  user_agent += "WIN ";
#elif defined(OS_LINUX)
  user_agent += "LINUX ";
#elif defined(OS_FREEBSD)
  user_agent += "FREEBSD ";
#elif defined(OS_OPENBSD)
  user_agent += "OPENBSD ";
#elif defined(OS_MACOSX)
  user_agent += "MAC ";
#endif
  chrome::VersionInfo version_info;
  if (!version_info.is_valid()) {
    DLOG(ERROR) << "Unable to create chrome::VersionInfo object";
    return user_agent;
  }

  user_agent += version_info.Version();
  user_agent += " (" + version_info.LastChange() + ")";
  if (!version_info.IsOfficialBuild())
    user_agent += "-devel";
  return user_agent;
}

void SyncBackendHost::Core::DoInitialize(const DoInitializeOptions& options) {
  DCHECK(MessageLoop::current() == host_->sync_thread_.message_loop());
  processing_passphrase_ = false;

  // Blow away the partial or corrupt sync data folder before doing any more
  // initialization, if necessary.
  if (options.delete_sync_data_folder) {
    DeleteSyncDataFolder();
  }

  // Make sure that the directory exists before initializing the backend.
  // If it already exists, this will do no harm.
  bool success = file_util::CreateDirectory(host_->sync_data_folder_path());
  DCHECK(success);

  sync_manager_.reset(new sync_api::SyncManager(name_)),
  sync_manager_->AddObserver(this);
  const FilePath& path_str = host_->sync_data_folder_path();
  success = sync_manager_->Init(
      path_str,
      options.service_url.host() + options.service_url.path(),
      options.service_url.EffectiveIntPort(),
      options.service_url.SchemeIsSecure(),
      host_->MakeHttpBridgeFactory(options.request_context_getter),
      host_,  // ModelSafeWorkerRegistrar.
      MakeUserAgentForSyncApi(),
      options.credentials,
      host_->sync_notifier_factory_.CreateSyncNotifier(),
      options.restored_key_for_bootstrapping,
      options.setup_for_test_mode);
  DCHECK(success) << "Syncapi initialization failed!";
}

void SyncBackendHost::Core::DoUpdateCredentials(
    const SyncCredentials& credentials) {
  DCHECK(MessageLoop::current() == host_->sync_thread_.message_loop());
  sync_manager_->UpdateCredentials(credentials);
}

void SyncBackendHost::Core::DoUpdateEnabledTypes() {
  DCHECK(MessageLoop::current() == host_->sync_thread_.message_loop());
  sync_manager_->UpdateEnabledTypes();
}

void SyncBackendHost::Core::DoStartSyncing() {
  DCHECK(MessageLoop::current() == host_->sync_thread_.message_loop());
  sync_manager_->StartSyncingNormally();
  if (deferred_nudge_for_cleanup_requested_)
    sync_manager_->RequestNudge(FROM_HERE);
  deferred_nudge_for_cleanup_requested_ = false;
}

void SyncBackendHost::Core::DoSetPassphrase(const std::string& passphrase,
                                            bool is_explicit) {
  DCHECK(MessageLoop::current() == host_->sync_thread_.message_loop());
  sync_manager_->SetPassphrase(passphrase, is_explicit);
}

bool SyncBackendHost::Core::processing_passphrase() const {
  DCHECK(MessageLoop::current() == host_->frontend_loop_);
  return processing_passphrase_;
}

void SyncBackendHost::Core::set_processing_passphrase() {
  DCHECK(MessageLoop::current() == host_->frontend_loop_);
  processing_passphrase_ = true;
}

void SyncBackendHost::Core::DoEncryptDataTypes(
    const syncable::ModelTypeSet& encrypted_types) {
  DCHECK(MessageLoop::current() == host_->sync_thread_.message_loop());
  sync_manager_->EncryptDataTypes(encrypted_types);
}

void SyncBackendHost::Core::DoRequestConfig(
    const syncable::ModelTypeBitSet& added_types,
    sync_api::ConfigureReason reason) {
  sync_manager_->RequestConfig(added_types, reason);
}

void SyncBackendHost::Core::DoStartConfiguration(Callback0::Type* callback) {
  sync_manager_->StartConfigurationMode(callback);
}

UIModelWorker* SyncBackendHost::ui_worker() {
  ModelSafeWorker* w = registrar_.workers[GROUP_UI];
  if (w == NULL)
    return NULL;
  if (w->GetModelSafeGroup() != GROUP_UI)
    NOTREACHED();
  return static_cast<UIModelWorker*>(w);
}

void SyncBackendHost::Core::DoShutdown(bool sync_disabled) {
  DCHECK(MessageLoop::current() == host_->sync_thread_.message_loop());

  save_changes_timer_.Stop();
  sync_manager_->Shutdown();  // Stops the SyncerThread.
  sync_manager_->RemoveObserver(this);
  DisconnectChildJsEventRouter();
  sync_manager_.reset();
  host_->ui_worker()->OnSyncerShutdownComplete();

  if (sync_disabled)
    DeleteSyncDataFolder();

  host_ = NULL;
}

ChangeProcessor* SyncBackendHost::Core::GetProcessor(
    syncable::ModelType model_type) {
  std::map<syncable::ModelType, ChangeProcessor*>::const_iterator it =
      host_->processors_.find(model_type);

  // Until model association happens for a datatype, it will not appear in
  // the processors list.  During this time, it is OK to drop changes on
  // the floor (since model association has not happened yet).  When the
  // data type is activated, model association takes place then the change
  // processor is added to the processors_ list.  This all happens on
  // the UI thread so we will never drop any changes after model
  // association.
  if (it == host_->processors_.end())
    return NULL;

  if (!IsCurrentThreadSafeForModel(model_type)) {
    NOTREACHED() << "Changes applied on wrong thread. Model: "
                 <<  syncable::ModelTypeToString(model_type);
    return NULL;
  }

  // Now that we're sure we're on the correct thread, we can access the
  // ChangeProcessor.
  ChangeProcessor* processor = it->second;

  // Ensure the change processor is willing to accept changes.
  if (!processor->IsRunning())
    return NULL;

  return processor;
}

void SyncBackendHost::Core::OnChangesApplied(
    syncable::ModelType model_type,
    const sync_api::BaseTransaction* trans,
    const sync_api::SyncManager::ChangeRecord* changes,
    int change_count) {
  if (!host_ || !host_->frontend_) {
    DCHECK(false) << "OnChangesApplied called after Shutdown?";
    return;
  }
  ChangeProcessor* processor = GetProcessor(model_type);
  if (!processor)
    return;

  processor->ApplyChangesFromSyncModel(trans, changes, change_count);
}

void SyncBackendHost::Core::OnChangesComplete(
    syncable::ModelType model_type) {
  if (!host_ || !host_->frontend_) {
    DCHECK(false) << "OnChangesComplete called after Shutdown?";
    return;
  }

  ChangeProcessor* processor = GetProcessor(model_type);
  if (!processor)
    return;

  // This call just notifies the processor that it can commit, it already
  // buffered any changes it plans to makes so needs no further information.
  processor->CommitChangesFromSyncModel();
}


void SyncBackendHost::Core::OnSyncCycleCompleted(
    const SyncSessionSnapshot* snapshot) {
  host_->frontend_loop_->PostTask(FROM_HERE, NewRunnableMethod(this,
      &Core::HandleSyncCycleCompletedOnFrontendLoop,
      new SyncSessionSnapshot(*snapshot)));
}

void SyncBackendHost::Core::HandleSyncCycleCompletedOnFrontendLoop(
    SyncSessionSnapshot* snapshot) {
  if (!host_ || !host_->frontend_)
    return;
  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);

  host_->last_snapshot_.reset(snapshot);

  const syncable::ModelTypeSet& to_migrate =
      snapshot->syncer_status.types_needing_local_migration;
  if (!to_migrate.empty())
    host_->frontend_->OnMigrationNeededForTypes(to_migrate);

  // If we are waiting for a configuration change, check here to see
  // if this sync cycle has initialized all of the types we've been
  // waiting for.
  if (host_->pending_download_state_.get()) {
    scoped_ptr<PendingConfigureDataTypesState> state(
        host_->pending_download_state_.release());
    bool found_all_added = true;
    syncable::ModelTypeSet::const_iterator it;
    for (it = state->initial_types.begin(); it != state->initial_types.end();
         ++it) {
      if (state->added_types.test(*it))
        found_all_added &= snapshot->initial_sync_ended.test(*it);
    }
    if (!found_all_added) {
      DLOG(WARNING) << "Update didn't return updates for all types requested."
                    << "Possible connection failure?";
    } else {
      state->ready_task->Run();
    }
  }

  if (host_->initialized())
    host_->frontend_->OnSyncCycleCompleted();
}

void SyncBackendHost::Core::OnInitializationComplete() {
  if (!host_ || !host_->frontend_)
    return;  // We may have been told to Shutdown before initialization
             // completed.

  // We could be on some random sync backend thread, so MessageLoop::current()
  // can definitely be null in here.
  host_->frontend_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this,
                        &Core::HandleInitializationCompletedOnFrontendLoop));

  // Initialization is complete, so we can schedule recurring SaveChanges.
  host_->sync_thread_.message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(this, &Core::StartSavingChanges));
}

void SyncBackendHost::Core::HandleInitializationCompletedOnFrontendLoop() {
  if (!host_)
    return;
  host_->HandleInitializationCompletedOnFrontendLoop();
}

void SyncBackendHost::HandleInitializationCompletedOnFrontendLoop() {
  if (!frontend_)
    return;

  bool setup_completed =
      profile_->GetPrefs()->GetBoolean(prefs::kSyncHasSetupCompleted);
  if (setup_completed || initialization_state_ == DOWNLOADING_NIGORI) {
    // Now that the syncapi is initialized, we can update the cryptographer
    // and can handle any ON_PASSPHRASE_REQUIRED notifications that may arise.
    // TODO(tim): Bug 87797. We should be able to RefreshEncryption
    // unconditionally as soon as the UI supports having this info early on.
    if (setup_completed)
      core_->sync_manager()->RefreshEncryption();
    initialization_state_ = INITIALIZED;
    frontend_->OnBackendInitialized();
    return;
  }

  initialization_state_ = DOWNLOADING_NIGORI;
  ConfigureDataTypes(
      DataTypeController::TypeMap(),
      syncable::ModelTypeSet(),
      sync_api::CONFIGURE_REASON_NEW_CLIENT,
      NewRunnableMethod(core_.get(),
          &SyncBackendHost::Core::HandleInitializationCompletedOnFrontendLoop),
      true);
}

bool SyncBackendHost::Core::IsCurrentThreadSafeForModel(
    syncable::ModelType model_type) {
  base::AutoLock lock(host_->registrar_lock_);

  browser_sync::ModelSafeRoutingInfo::const_iterator routing_it =
      host_->registrar_.routing_info.find(model_type);
  if (routing_it == host_->registrar_.routing_info.end())
    return false;
  browser_sync::ModelSafeGroup group = routing_it->second;
  WorkerMap::const_iterator worker_it = host_->registrar_.workers.find(group);
  if (worker_it == host_->registrar_.workers.end())
    return false;
  ModelSafeWorker* worker = worker_it->second;
  return worker->CurrentThreadIsWorkThread();
}

void SyncBackendHost::Core::OnAuthError(const AuthError& auth_error) {
  // Post to our core loop so we can modify state. Could be on another thread.
  host_->frontend_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &Core::HandleAuthErrorEventOnFrontendLoop,
      auth_error));
}

void SyncBackendHost::Core::OnPassphraseRequired(
    sync_api::PassphraseRequiredReason reason) {
  host_->frontend_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &Core::NotifyPassphraseRequired, reason));
}

void SyncBackendHost::Core::OnPassphraseAccepted(
    const std::string& bootstrap_token) {
  host_->frontend_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &Core::NotifyPassphraseAccepted,
          bootstrap_token));
}

void SyncBackendHost::Core::OnStopSyncingPermanently() {
  host_->frontend_loop_->PostTask(FROM_HERE, NewRunnableMethod(this,
      &Core::HandleStopSyncingPermanentlyOnFrontendLoop));
}

void SyncBackendHost::Core::OnUpdatedToken(const std::string& token) {
  host_->frontend_loop_->PostTask(FROM_HERE, NewRunnableMethod(this,
      &Core::NotifyUpdatedToken, token));
}

void SyncBackendHost::Core::OnClearServerDataSucceeded() {
  host_->frontend_loop_->PostTask(FROM_HERE, NewRunnableMethod(this,
      &Core::HandleClearServerDataSucceededOnFrontendLoop));
}

void SyncBackendHost::Core::OnClearServerDataFailed() {
  host_->frontend_loop_->PostTask(FROM_HERE, NewRunnableMethod(this,
      &Core::HandleClearServerDataFailedOnFrontendLoop));
}

void SyncBackendHost::Core::OnEncryptionComplete(
    const syncable::ModelTypeSet& encrypted_types) {
  host_->frontend_loop_->PostTask(
      FROM_HERE,
      NewRunnableMethod(this, &Core::NotifyEncryptionComplete,
                        encrypted_types));
}

void SyncBackendHost::Core::RouteJsEvent(
    const std::string& name, const JsEventDetails& details) {
  host_->frontend_loop_->PostTask(
      FROM_HERE, NewRunnableMethod(
          this, &Core::RouteJsEventOnFrontendLoop, name, details));
}

void SyncBackendHost::Core::RouteJsMessageReply(
    const std::string& name, const JsArgList& args,
    const JsEventHandler* target) {
  host_->frontend_loop_->PostTask(
      FROM_HERE, NewRunnableMethod(
          this, &Core::RouteJsMessageReplyOnFrontendLoop,
          name, args, target));
}

void SyncBackendHost::Core::HandleStopSyncingPermanentlyOnFrontendLoop() {
  if (!host_ || !host_->frontend_)
    return;
  host_->frontend_->OnStopSyncingPermanently();
}

void SyncBackendHost::Core::HandleClearServerDataSucceededOnFrontendLoop() {
  if (!host_ || !host_->frontend_)
    return;
  host_->frontend_->OnClearServerDataSucceeded();
}

void SyncBackendHost::Core::HandleClearServerDataFailedOnFrontendLoop() {
  if (!host_ || !host_->frontend_)
    return;
  host_->frontend_->OnClearServerDataFailed();
}

void SyncBackendHost::Core::HandleAuthErrorEventOnFrontendLoop(
    const GoogleServiceAuthError& new_auth_error) {
  if (!host_ || !host_->frontend_)
    return;

  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);

  host_->last_auth_error_ = new_auth_error;
  host_->frontend_->OnAuthError();
}

void SyncBackendHost::Core::RouteJsEventOnFrontendLoop(
    const std::string& name, const JsEventDetails& details) {
  if (!host_ || !parent_router_)
    return;

  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);

  parent_router_->RouteJsEvent(name, details);
}

void SyncBackendHost::Core::RouteJsMessageReplyOnFrontendLoop(
    const std::string& name, const JsArgList& args,
    const JsEventHandler* target) {
  if (!host_ || !parent_router_)
    return;

  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);

  parent_router_->RouteJsMessageReply(name, args, target);
}

void SyncBackendHost::Core::StartSavingChanges() {
  save_changes_timer_.Start(
      base::TimeDelta::FromSeconds(kSaveChangesIntervalSeconds),
      this, &Core::SaveChanges);
}

void SyncBackendHost::Core::DoRequestNudge(
    const tracked_objects::Location& nudge_location) {
  sync_manager_->RequestNudge(nudge_location);
}

void SyncBackendHost::Core::DoRequestClearServerData() {
  sync_manager_->RequestClearServerData();
}

void SyncBackendHost::Core::SaveChanges() {
  sync_manager_->SaveChanges();
}

void SyncBackendHost::Core::DeleteSyncDataFolder() {
  if (file_util::DirectoryExists(host_->sync_data_folder_path())) {
    if (!file_util::Delete(host_->sync_data_folder_path(), true))
      LOG(DFATAL) << "Could not delete the Sync Data folder.";
  }
}

void SyncBackendHost::Core::SetParentJsEventRouter(JsEventRouter* router) {
  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);
  DCHECK(router);
  parent_router_ = router;
  MessageLoop* sync_loop = host_->sync_thread_.message_loop();
  CHECK(sync_loop);
  sync_loop->PostTask(
      FROM_HERE,
      NewRunnableMethod(this,
                        &SyncBackendHost::Core::ConnectChildJsEventRouter));
}

void SyncBackendHost::Core::RemoveParentJsEventRouter() {
  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);
  parent_router_ = NULL;
  MessageLoop* sync_loop = host_->sync_thread_.message_loop();
  CHECK(sync_loop);
  sync_loop->PostTask(
      FROM_HERE,
      NewRunnableMethod(this,
                        &SyncBackendHost::Core::DisconnectChildJsEventRouter));
}

const JsEventRouter* SyncBackendHost::Core::GetParentJsEventRouter() const {
  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);
  return parent_router_;
}

void SyncBackendHost::Core::ProcessMessage(
    const std::string& name, const JsArgList& args,
    const JsEventHandler* sender) {
  DCHECK_EQ(MessageLoop::current(), host_->frontend_loop_);
  MessageLoop* sync_loop = host_->sync_thread_.message_loop();
  CHECK(sync_loop);
  sync_loop->PostTask(
      FROM_HERE,
      NewRunnableMethod(this,
                        &SyncBackendHost::Core::DoProcessMessage,
                        name, args, sender));
}

void SyncBackendHost::Core::ConnectChildJsEventRouter() {
  DCHECK_EQ(MessageLoop::current(), host_->sync_thread_.message_loop());
  // We need this check since AddObserver() can be called at most once
  // for a given observer.
  if (!sync_manager_->GetJsBackend()->GetParentJsEventRouter()) {
    sync_manager_->GetJsBackend()->SetParentJsEventRouter(this);
    sync_manager_->AddObserver(&sync_manager_observer_);
  }
}

void SyncBackendHost::Core::DisconnectChildJsEventRouter() {
  DCHECK_EQ(MessageLoop::current(), host_->sync_thread_.message_loop());
  sync_manager_->GetJsBackend()->RemoveParentJsEventRouter();
  sync_manager_->RemoveObserver(&sync_manager_observer_);
}

void SyncBackendHost::Core::DoProcessMessage(
    const std::string& name, const JsArgList& args,
    const JsEventHandler* sender) {
  DCHECK_EQ(MessageLoop::current(), host_->sync_thread_.message_loop());
  sync_manager_->GetJsBackend()->ProcessMessage(name, args, sender);
}

void SyncBackendHost::Core::DeferNudgeForCleanup() {
  DCHECK_EQ(MessageLoop::current(), host_->sync_thread_.message_loop());
  deferred_nudge_for_cleanup_requested_ = true;
}

}  // namespace browser_sync
