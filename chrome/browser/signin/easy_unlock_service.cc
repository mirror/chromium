// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/easy_unlock_service.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/field_trial.h"
#include "base/prefs/pref_service.h"
#include "base/prefs/scoped_user_pref_update.h"
#include "base/values.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/easy_unlock_screenlock_state_handler.h"
#include "chrome/browser/signin/easy_unlock_service_factory.h"
#include "chrome/browser/signin/easy_unlock_service_observer.h"
#include "chrome/browser/signin/easy_unlock_toggle_flow.h"
#include "chrome/browser/signin/screenlock_bridge.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"
#include "grit/browser_resources.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "components/user_manager/user_manager.h"
#endif

namespace {

// Key name of the local device permit record dictonary in kEasyUnlockPairing.
const char kKeyPermitAccess[] = "permitAccess";

// Key name of the remote device list in kEasyUnlockPairing.
const char kKeyDevices[] = "devices";

// Key name of the phone public key in a device dictionary.
const char kKeyPhoneId[] = "permitRecord.id";

extensions::ComponentLoader* GetComponentLoader(
    content::BrowserContext* context) {
  extensions::ExtensionSystem* extension_system =
      extensions::ExtensionSystem::Get(context);
  ExtensionService* extension_service = extension_system->extension_service();
  return extension_service->component_loader();
}

}  // namespace

// static
EasyUnlockService* EasyUnlockService::Get(Profile* profile) {
  return EasyUnlockServiceFactory::GetForProfile(profile);
}

class EasyUnlockService::BluetoothDetector
    : public device::BluetoothAdapter::Observer {
 public:
  explicit BluetoothDetector(EasyUnlockService* service)
      : service_(service),
        weak_ptr_factory_(this) {
  }

  virtual ~BluetoothDetector() {
    if (adapter_)
      adapter_->RemoveObserver(this);
  }

  void Initialize() {
    if (!device::BluetoothAdapterFactory::IsBluetoothAdapterAvailable())
      return;

    device::BluetoothAdapterFactory::GetAdapter(
        base::Bind(&BluetoothDetector::OnAdapterInitialized,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  bool IsPresent() const {
    return adapter_ && adapter_->IsPresent();
  }

  // device::BluetoothAdapter::Observer:
  virtual void AdapterPresentChanged(device::BluetoothAdapter* adapter,
                                     bool present) OVERRIDE {
    service_->OnBluetoothAdapterPresentChanged();
  }

 private:
  void OnAdapterInitialized(scoped_refptr<device::BluetoothAdapter> adapter) {
    adapter_ = adapter;
    adapter_->AddObserver(this);
    service_->OnBluetoothAdapterPresentChanged();
  }

  // Owner of this class and should out-live this class.
  EasyUnlockService* service_;
  scoped_refptr<device::BluetoothAdapter> adapter_;
  base::WeakPtrFactory<BluetoothDetector> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothDetector);
};

EasyUnlockService::EasyUnlockService(Profile* profile)
    : profile_(profile),
      bluetooth_detector_(new BluetoothDetector(this)),
      turn_off_flow_status_(IDLE),
      weak_ptr_factory_(this) {
  extensions::ExtensionSystem::Get(profile_)->ready().Post(
      FROM_HERE,
      base::Bind(&EasyUnlockService::Initialize,
                 weak_ptr_factory_.GetWeakPtr()));
}

EasyUnlockService::~EasyUnlockService() {
}

// static
void EasyUnlockService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kEasyUnlockEnabled,
      false,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kEasyUnlockShowTutorial,
      true,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterDictionaryPref(
      prefs::kEasyUnlockPairing,
      new base::DictionaryValue(),
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kEasyUnlockAllowed,
      true,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
}

void EasyUnlockService::LaunchSetup() {
  ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  const extensions::Extension* extension =
      service->GetExtensionById(extension_misc::kEasyUnlockAppId, false);

  OpenApplication(AppLaunchParams(
      profile_, extension, extensions::LAUNCH_CONTAINER_WINDOW, NEW_WINDOW));
}

bool EasyUnlockService::IsAllowed() {
#if defined(OS_CHROMEOS)
  if (!user_manager::UserManager::Get()->IsLoggedInAsRegularUser())
    return false;

  if (!chromeos::ProfileHelper::IsPrimaryProfile(profile_))
    return false;

  if (!profile_->GetPrefs()->GetBoolean(prefs::kEasyUnlockAllowed))
    return false;

  // Respect existing policy and skip finch test.
  if (!profile_->GetPrefs()->IsManagedPreference(prefs::kEasyUnlockAllowed)) {
    // It is disabled when the trial exists and is in "Disable" group.
    if (base::FieldTrialList::FindFullName("EasyUnlock") == "Disable")
      return false;
  }

  if (!bluetooth_detector_->IsPresent())
    return false;

  return true;
#else
  // TODO(xiyuan): Revisit when non-chromeos platforms are supported.
  return false;
#endif
}

EasyUnlockScreenlockStateHandler*
    EasyUnlockService::GetScreenlockStateHandler() {
  if (!IsAllowed())
    return NULL;
  if (!screenlock_state_handler_) {
    screenlock_state_handler_.reset(new EasyUnlockScreenlockStateHandler(
        ScreenlockBridge::GetAuthenticatedUserEmail(profile_),
        profile_->GetPrefs(),
        ScreenlockBridge::Get()));
  }
  return screenlock_state_handler_.get();
}

const base::DictionaryValue* EasyUnlockService::GetPermitAccess() const {
  const base::DictionaryValue* pairing_dict =
      profile_->GetPrefs()->GetDictionary(prefs::kEasyUnlockPairing);
  const base::DictionaryValue* permit_dict = NULL;
  if (pairing_dict &&
      pairing_dict->GetDictionary(kKeyPermitAccess, &permit_dict)) {
    return permit_dict;
  }

  return NULL;
}

void EasyUnlockService::SetPermitAccess(const base::DictionaryValue& permit) {
  DictionaryPrefUpdate pairing_update(profile_->GetPrefs(),
                                      prefs::kEasyUnlockPairing);
  pairing_update->SetWithoutPathExpansion(kKeyPermitAccess, permit.DeepCopy());
}

void EasyUnlockService::ClearPermitAccess() {
  DictionaryPrefUpdate pairing_update(profile_->GetPrefs(),
                                      prefs::kEasyUnlockPairing);
  pairing_update->RemoveWithoutPathExpansion(kKeyPermitAccess, NULL);
}

const base::ListValue* EasyUnlockService::GetRemoteDevices() const {
  const base::DictionaryValue* pairing_dict =
      profile_->GetPrefs()->GetDictionary(prefs::kEasyUnlockPairing);
  const base::ListValue* devices = NULL;
  if (pairing_dict && pairing_dict->GetList(kKeyDevices, &devices)) {
    return devices;
  }

  return NULL;
}

void EasyUnlockService::SetRemoteDevices(const base::ListValue& devices) {
  DictionaryPrefUpdate pairing_update(profile_->GetPrefs(),
                                      prefs::kEasyUnlockPairing);
  pairing_update->SetWithoutPathExpansion(kKeyDevices, devices.DeepCopy());
}

void EasyUnlockService::ClearRemoteDevices() {
  DictionaryPrefUpdate pairing_update(profile_->GetPrefs(),
                                      prefs::kEasyUnlockPairing);
  pairing_update->RemoveWithoutPathExpansion(kKeyDevices, NULL);
}

void EasyUnlockService::AddObserver(EasyUnlockServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void EasyUnlockService::RemoveObserver(EasyUnlockServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void EasyUnlockService::RunTurnOffFlow() {
  if (turn_off_flow_status_ == PENDING)
    return;

  SetTurnOffFlowStatus(PENDING);

  // Currently there should only be one registered phone.
  // TODO(xiyuan): Revisit this when server supports toggle for all or
  // there are multiple phones.
  const base::DictionaryValue* pairing_dict =
      profile_->GetPrefs()->GetDictionary(prefs::kEasyUnlockPairing);
  const base::ListValue* devices_list = NULL;
  const base::DictionaryValue* first_device = NULL;
  std::string phone_public_key;
  if (!pairing_dict || !pairing_dict->GetList(kKeyDevices, &devices_list) ||
      !devices_list || !devices_list->GetDictionary(0, &first_device) ||
      !first_device ||
      !first_device->GetString(kKeyPhoneId, &phone_public_key)) {
    LOG(WARNING) << "Bad easy unlock pairing data, wiping out local data";
    OnTurnOffFlowFinished(true);
    return;
  }

  turn_off_flow_.reset(new EasyUnlockToggleFlow(
      profile_,
      phone_public_key,
      false,
      base::Bind(&EasyUnlockService::OnTurnOffFlowFinished,
                 base::Unretained(this))));
  turn_off_flow_->Start();
}

void EasyUnlockService::ResetTurnOffFlow() {
  turn_off_flow_.reset();
  SetTurnOffFlowStatus(IDLE);
}

void EasyUnlockService::Initialize() {
  registrar_.Init(profile_->GetPrefs());
  registrar_.Add(
      prefs::kEasyUnlockAllowed,
      base::Bind(&EasyUnlockService::OnPrefsChanged, base::Unretained(this)));
  OnPrefsChanged();

#if defined(OS_CHROMEOS)
  // Only start Bluetooth detection for ChromeOS since the feature is
  // only offered on ChromeOS. Enabling this on non-ChromeOS platforms
  // previously introduced a performance regression: http://crbug.com/404482
  // Make sure not to reintroduce a performance regression if re-enabling on
  // additional platforms.
  // TODO(xiyuan): Revisit when non-chromeos platforms are supported.
  bluetooth_detector_->Initialize();
#endif  // defined(OS_CHROMEOS)
}

void EasyUnlockService::LoadApp() {
  DCHECK(IsAllowed());

#if defined(GOOGLE_CHROME_BUILD)
  base::FilePath easy_unlock_path;
#if defined(OS_CHROMEOS)
  easy_unlock_path = base::FilePath("/usr/share/chromeos-assets/easy_unlock");
#endif  // defined(OS_CHROMEOS)

#ifndef NDEBUG
  // Only allow app path override switch for debug build.
  const CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEasyUnlockAppPath)) {
    easy_unlock_path =
        command_line->GetSwitchValuePath(switches::kEasyUnlockAppPath);
  }
#endif  // !defined(NDEBUG)

  if (!easy_unlock_path.empty()) {
    extensions::ComponentLoader* loader = GetComponentLoader(profile_);
    if (!loader->Exists(extension_misc::kEasyUnlockAppId))
      loader->Add(IDR_EASY_UNLOCK_MANIFEST, easy_unlock_path);
  }
#endif  // defined(GOOGLE_CHROME_BUILD)
}

void EasyUnlockService::UnloadApp() {
  extensions::ComponentLoader* loader = GetComponentLoader(profile_);
  if (loader->Exists(extension_misc::kEasyUnlockAppId))
    loader->Remove(extension_misc::kEasyUnlockAppId);
}

void EasyUnlockService::UpdateAppState() {
  if (IsAllowed()) {
    LoadApp();
  } else {
    UnloadApp();
    // Reset the screenlock state handler to make sure Screenlock state set
    // by Easy Unlock app is reset.
    screenlock_state_handler_.reset();
  }
}

void EasyUnlockService::OnPrefsChanged() {
  UpdateAppState();
}

void EasyUnlockService::OnBluetoothAdapterPresentChanged() {
  UpdateAppState();
}

void EasyUnlockService::SetTurnOffFlowStatus(TurnOffFlowStatus status) {
  turn_off_flow_status_ = status;
  FOR_EACH_OBSERVER(
      EasyUnlockServiceObserver, observers_, OnTurnOffOperationStatusChanged());
}

void EasyUnlockService::OnTurnOffFlowFinished(bool success) {
  turn_off_flow_.reset();

  if (!success) {
    SetTurnOffFlowStatus(FAIL);
    return;
  }

  ClearRemoteDevices();
  SetTurnOffFlowStatus(IDLE);

  // Make sure lock screen state set by the extension gets reset.
  screenlock_state_handler_.reset();

  if (GetComponentLoader(profile_)->Exists(extension_misc::kEasyUnlockAppId)) {
    extensions::ExtensionSystem* extension_system =
        extensions::ExtensionSystem::Get(profile_);
    extension_system->extension_service()->ReloadExtension(
        extension_misc::kEasyUnlockAppId);
  }
}
