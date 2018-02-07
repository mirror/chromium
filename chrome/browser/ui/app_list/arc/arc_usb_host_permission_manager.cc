// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_usb_host_permission_manager.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_usb_host_permission_manager_factory.h"
#include "components/arc/usb/usb_host_bridge.h"

namespace arc {

namespace {

std::string GetAppIdFromPackageName(const std::string& package_name,
                                    const ArcAppListPrefs* arc_app_list_prefs) {
  DCHECK(arc_app_list_prefs);

  // For app icon and app name in UI dialog, use first matching activity for the
  // requesting package.
  std::vector<std::string> app_ids = arc_app_list_prefs->GetAppIds();
  for (const auto& app_id : app_ids) {
    auto app_info = arc_app_list_prefs->GetApp(app_id);
    if (app_info->package_name == package_name)
      return app_id;
  }
  return std::string();
}

bool HasUsbAccessPermissionLocal(
    const std::vector<std::unique_ptr<
        ArcUsbHostPermissionManager::UsbDeviceEntry>>& usb_device_records,
    const ArcUsbHostPermissionManager::UsbDeviceEntry* usb_device_entry) {
  for (const auto& usb_device_record : usb_device_records) {
    if (usb_device_record->Matches(usb_device_entry))
      return true;
  }
  return false;
}

}  // namespace

// UsbPermissionRequest
ArcUsbHostPermissionManager::UsbPermissionRequest::UsbPermissionRequest(
    const std::string& package_name)
    : package_name(package_name) {}

ArcUsbHostPermissionManager::UsbPermissionRequest::~UsbPermissionRequest() {}

std::unique_ptr<ArcUsbHostPermissionManager::UsbPermissionRequest>
ArcUsbHostPermissionManager::UsbPermissionRequest::
    CreateUsbScanDeviceListPermissionRequest(const std::string& package_name) {
  auto usb_permission_request =
      base::MakeUnique<ArcUsbHostPermissionManager::UsbPermissionRequest>(
          package_name);
  return usb_permission_request;
}

std::unique_ptr<ArcUsbHostPermissionManager::UsbPermissionRequest>
ArcUsbHostPermissionManager::UsbPermissionRequest::
    CreateUsbAccessPermissionRequest(
        const std::string& package_name,
        std::unique_ptr<ArcUsbHostPermissionManager::UsbDeviceEntry>
            usb_device_entry,
        RequestPermissionCallback callback) {
  auto usb_permission_request =
      base::MakeUnique<ArcUsbHostPermissionManager::UsbPermissionRequest>(
          package_name);
  usb_permission_request->usb_device_entry = std::move(usb_device_entry);
  usb_permission_request->callback = std::move(callback);
  return usb_permission_request;
}

// UsbDeviceEntry
ArcUsbHostPermissionManager::UsbDeviceEntry::UsbDeviceEntry(
    const std::string& guid,
    const base::string16& device_name,
    const base::string16& serial_number,
    uint16_t vendor_id,
    uint16_t product_id)
    : guid(guid),
      device_name(device_name),
      serial_number(serial_number),
      vendor_id(vendor_id),
      product_id(product_id) {}

ArcUsbHostPermissionManager::UsbDeviceEntry::UsbDeviceEntry(
    const ArcUsbHostPermissionManager::UsbDeviceEntry& other)
    : guid(other.guid),
      device_name(other.device_name),
      serial_number(other.serial_number),
      vendor_id(other.vendor_id),
      product_id(other.product_id) {}

bool ArcUsbHostPermissionManager::UsbDeviceEntry::IsPersistent() const {
  return !serial_number.empty();
}

bool ArcUsbHostPermissionManager::UsbDeviceEntry::Matches(
    const UsbDeviceEntry* other) const {
  if (IsPersistent() && other->IsPersistent()) {
    return serial_number == other->serial_number &&
           vendor_id == other->vendor_id && product_id == other->product_id;
  } else {
    return guid == other->guid;
  }
}

// ArcUsbHostPermissionManager
// static
ArcUsbHostPermissionManager* ArcUsbHostPermissionManager::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcUsbHostPermissionManagerFactory::GetForBrowserContext(context);
}

// static
ArcUsbHostPermissionManager* ArcUsbHostPermissionManager::Create(
    content::BrowserContext* context) {
  ArcAppListPrefs* arc_app_list_prefs = ArcAppListPrefs::Get(context);
  if (!arc_app_list_prefs)
    return nullptr;

  ArcUsbHostBridge* arc_usb_host_bridge =
      ArcUsbHostBridge::GetForBrowserContext(context);
  if (!arc_usb_host_bridge)
    return nullptr;

  return new ArcUsbHostPermissionManager(
      static_cast<Profile*>(context), arc_app_list_prefs, arc_usb_host_bridge);
}

ArcUsbHostPermissionManager::ArcUsbHostPermissionManager(
    Profile* profile,
    ArcAppListPrefs* arc_app_list_prefs,
    ArcUsbHostBridge* arc_usb_host_bridge)
    : profile_(profile),
      arc_app_list_prefs_(arc_app_list_prefs),
      arc_usb_host_bridge_(arc_usb_host_bridge),
      weak_ptr_factory_(this) {
  ReStorePermissionFromChromePrefs();
  arc_app_list_prefs_->AddObserver(this);
  arc_usb_host_bridge_->SetUiDelegate(this);
}

ArcUsbHostPermissionManager::~ArcUsbHostPermissionManager() {
  arc_app_list_prefs_->RemoveObserver(this);
  arc_usb_host_bridge_->ClearUiDelegate();
}

void ArcUsbHostPermissionManager::ReStorePermissionFromChromePrefs() {
  // Restore scan devicelist permissions.

  // Restore persistent device access permissions.
}

void ArcUsbHostPermissionManager::RequestUsbScanDeviceListPermission(
    const std::string& package_name,
    RequestPermissionCallback callback) {
  if (HasUsbScanDeviceListPermission(package_name)) {
    std::move(callback).Run(true);
    return;
  } else {
    // Return ASAP to stop app from ANR. Granted permission will apply when
    // next time app tries to get device list.
    std::move(callback).Run(false);
  }

  std::unique_ptr<UsbPermissionRequest> usb_permission_request =
      UsbPermissionRequest::CreateUsbScanDeviceListPermissionRequest(
          package_name);
  TryProcessNextPermissionRequest(std::move(usb_permission_request));
}

void ArcUsbHostPermissionManager::RequestUsbAccessPermission(
    const std::string& package_name,
    const std::string& guid,
    const base::string16& device_name,
    const base::string16& serial_number,
    uint16_t vendor_id,
    uint16_t product_id,
    RequestPermissionCallback callback) {
  auto usb_device_entry = base::MakeUnique<UsbDeviceEntry>(
      guid, device_name, serial_number, vendor_id, product_id);
  if (HasUsbAccessPermission(package_name, usb_device_entry.get())) {
    std::move(callback).Run(true);
    return;
  }

  std::unique_ptr<UsbPermissionRequest> usb_permission_request =
      UsbPermissionRequest::CreateUsbAccessPermissionRequest(
          package_name, std::move(usb_device_entry), std::move(callback));
  TryProcessNextPermissionRequest(std::move(usb_permission_request));
}

bool ArcUsbHostPermissionManager::HasUsbAccessPermission(
    const std::string& package_name,
    const std::string& guid,
    const base::string16& serial_number,
    uint16_t vendor_id,
    uint16_t product_id) {
  UsbDeviceEntry usb_device_entry(guid, base::string16(), serial_number,
                                  vendor_id, product_id);
  return HasUsbAccessPermission(package_name, &usb_device_entry);
}

void ArcUsbHostPermissionManager::DeviceRemoved(const std::string& guid) {
  // Remove pending requests.
  pending_requests_.erase(
      std::remove_if(
          pending_requests_.begin(), pending_requests_.end(),
          [guid](
              std::unique_ptr<UsbPermissionRequest>& usb_permission_request) {
            return usb_permission_request->usb_device_entry &&
                   usb_permission_request->usb_device_entry->guid == guid;
          }),
      pending_requests_.end());
  // Remove runtime permissions.
  for (auto& iter : ephemeral_usb_access_permission_dict_) {
    auto& ephemeral_usb_devices = iter.second;
    ephemeral_usb_devices.erase(
        std::remove_if(
            ephemeral_usb_devices.begin(), ephemeral_usb_devices.end(),
            [guid](const std::unique_ptr<UsbDeviceEntry>& usb_device) {
              return usb_device->guid == guid;
            }),
        ephemeral_usb_devices.end());
  }
}

void ArcUsbHostPermissionManager::OnPackageRemoved(
    const std::string& package_name,
    bool uninstalled) {
  // Remove pending requests.
  pending_requests_.erase(
      std::remove_if(
          pending_requests_.begin(), pending_requests_.end(),
          [package_name](
              std::unique_ptr<UsbPermissionRequest>& usb_permission_request) {
            return usb_permission_request->package_name == package_name;
          }),
      pending_requests_.end());
  // Remove runtime permissions.
  usb_scan_devicelist_permission_dict_.erase(package_name);
  persistent_usb_access_permission_dict_.erase(package_name);
  ephemeral_usb_access_permission_dict_.erase(package_name);
}

void ArcUsbHostPermissionManager::TryProcessNextPermissionRequest(
    std::unique_ptr<UsbPermissionRequest> usb_permission_request) {
  if (usb_permission_request)
    pending_requests_.push_back(std::move(usb_permission_request));

  if (!ui_dialog_available_ || pending_requests_.empty())
    return;

  ui_dialog_available_ = false;

  const auto& usb_permission_request_iter = pending_requests_.begin();
  const std::string& package_name =
      (*usb_permission_request_iter)->package_name;
  current_requesting_package_ = package_name;

  UsbDeviceEntry* usb_device_entry =
      (*usb_permission_request_iter)->usb_device_entry
          ? (*usb_permission_request_iter)->usb_device_entry.get()
          : nullptr;
  if (usb_device_entry)
    current_requesting_guid_ = usb_device_entry->guid;
  else
    current_requesting_guid_.clear();

  auto app_id = GetAppIdFromPackageName(package_name, arc_app_list_prefs_);
  // App is uninstalled during the process.
  if (app_id.empty()) {
    OnUsbPermissionReceived(false);
    return;
  }

  auto app_name = arc_app_list_prefs_->GetApp(app_id)->name;

  if (!usb_device_entry) {
    if (HasUsbScanDeviceListPermission(package_name)) {
      OnUsbPermissionReceived(true);
    } else {
      ShowArcUsbScanDeviceListPermissionDialog(
          profile_, app_id,
          base::BindOnce(&ArcUsbHostPermissionManager::OnUsbPermissionReceived,
                         weak_ptr_factory_.GetWeakPtr()));
    }
  } else {
    if (HasUsbAccessPermission(package_name, usb_device_entry)) {
      OnUsbPermissionReceived(true);
    } else {
      ShowArcUsbAccessPermissionDialog(
          profile_, app_id, usb_device_entry->device_name,
          base::BindOnce(&ArcUsbHostPermissionManager::OnUsbPermissionReceived,
                         weak_ptr_factory_.GetWeakPtr()));
    }
  }
}

bool ArcUsbHostPermissionManager::HasUsbScanDeviceListPermission(
    const std::string& package_name) const {
  return usb_scan_devicelist_permission_dict_.count(package_name);
}

bool ArcUsbHostPermissionManager::HasUsbAccessPermission(
    const std::string& package_name,
    const UsbDeviceEntry* usb_device_entry) {
  if (usb_device_entry->IsPersistent()) {
    return HasUsbAccessPermissionLocal(
        persistent_usb_access_permission_dict_[package_name], usb_device_entry);
  } else {
    return HasUsbAccessPermissionLocal(
        ephemeral_usb_access_permission_dict_[package_name], usb_device_entry);
  }
}

void ArcUsbHostPermissionManager::ClearPermissionRequests() {
  pending_requests_.clear();
  current_requesting_package_.clear();
  current_requesting_guid_.clear();
}

void ArcUsbHostPermissionManager::OnUsbPermissionReceived(bool accept) {
  ui_dialog_available_ = true;
  // This happens when pending request are cleared while UI selects permission
  // UI dialog. Ignore current callback as it's outdated.
  if (pending_requests_.empty()) {
    TryProcessNextPermissionRequest(nullptr);
    return;
  }

  const auto& usb_permission_request_iter = pending_requests_.begin();
  const std::string& package_name =
      (*usb_permission_request_iter)->package_name;
  UsbDeviceEntry* usb_device_entry =
      (*usb_permission_request_iter)->usb_device_entry
          ? (*usb_permission_request_iter)->usb_device_entry.get()
          : nullptr;

  // If the package can is uninstalled while user clicks permisison UI dialog.
  // Ignore current callback as it's outdated.
  if (package_name != current_requesting_package_) {
    TryProcessNextPermissionRequest(nullptr);
    return;
  }

  // If the USB device is removed while user clicks permission UI dialog.
  // Ignore current callback as it's outdated.
  if (usb_device_entry && usb_device_entry->guid != current_requesting_guid_) {
    TryProcessNextPermissionRequest(nullptr);
    return;
  }

  if (usb_device_entry) {
    std::move((*usb_permission_request_iter)->callback).Run(accept);
    TryUpdateArcUsbAccessPermission(package_name, usb_device_entry, accept);
  } else {
    TryUpdateArcUsbScanDeviceListPermission(package_name, accept);
  }

  pending_requests_.erase(usb_permission_request_iter);
  TryProcessNextPermissionRequest(nullptr);
}

void ArcUsbHostPermissionManager::TryUpdateArcUsbScanDeviceListPermission(
    const std::string& package_name,
    bool accept) {
  // Currently we don't keep denied request. But keep the option open.
  if (!accept)
    return;

  // Record already exsits.
  if (HasUsbScanDeviceListPermission(package_name))
    return;

  usb_scan_devicelist_permission_dict_.insert(package_name);
  // TODO update Chrome prefs.
}

void ArcUsbHostPermissionManager::TryUpdateArcUsbAccessPermission(
    const std::string& package_name,
    const UsbDeviceEntry* usb_device_entry,
    bool accept) {
  // Currently we don't keep denied request. But keep the option open.
  if (!accept)
    return;

  // Record already exsits.
  if (HasUsbAccessPermission(package_name, usb_device_entry))
    return;

  if (usb_device_entry->IsPersistent()) {
    persistent_usb_access_permission_dict_[package_name].push_back(
        base::MakeUnique<UsbDeviceEntry>(*usb_device_entry));
    // TODO update Chrome prefs.
  } else {
    ephemeral_usb_access_permission_dict_[package_name].push_back(
        base::MakeUnique<UsbDeviceEntry>(*usb_device_entry));
  }
}

}  // namespace arc
