// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_ARC_ARC_USB_HOST_PERMISSION_MANAGER_H_
#define CHROME_BROWSER_UI_APP_LIST_ARC_ARC_USB_HOST_PERMISSION_MANAGER_H_

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "components/arc/usb/usb_host_ui_delegate.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace arc {

class ArcUsbHostBridge;

using ArcUsbConfirmCallback = base::OnceCallback<void(bool accept)>;

void ShowArcUsbScanDeviceListPermissionDialog(Profile* profile,
                                              const std::string& app_id,
                                              ArcUsbConfirmCallback callback);

void ShowArcUsbAccessPermissionDialog(Profile* profile,
                                      const std::string& app_id,
                                      const base::string16& device_name,
                                      ArcUsbConfirmCallback callback);

class ArcUsbHostPermissionManager : public ArcAppListPrefs::Observer,
                                    public ArcUsbHostUiDelegate,
                                    public KeyedService {
 public:
  struct UsbDeviceEntry {
    UsbDeviceEntry(const std::string& guid,
                   const base::string16& device_name,
                   const base::string16& serial_number,
                   uint16_t vendor_id,
                   uint16_t product_id);
    UsbDeviceEntry(const UsbDeviceEntry& other);
    UsbDeviceEntry& operator=(const UsbDeviceEntry& other);

    bool IsPersistent() const;
    bool Matches(const UsbDeviceEntry* other) const;

    // This field can be null if device is persistent and the entry is restored
    // from Chrome prefs. But it can not be null if the entry is not persistent
    // or it is in PermissionRequest.
    const std::string guid;
    const base::string16 device_name;
    // This field is null if device is not persistent.
    const base::string16 serial_number;
    const uint16_t vendor_id;
    const uint16_t product_id;
  };

  struct UsbPermissionRequest {
   public:
    explicit UsbPermissionRequest(const std::string& package_name);
    ~UsbPermissionRequest();
    static std::unique_ptr<UsbPermissionRequest>
    CreateUsbScanDeviceListPermissionRequest(const std::string& package_name);
    static std::unique_ptr<UsbPermissionRequest>
    CreateUsbAccessPermissionRequest(
        const std::string& package_name,
        std::unique_ptr<UsbDeviceEntry> usb_device_entry,
        RequestPermissionCallback callback);

    const std::string package_name;
    std::unique_ptr<UsbDeviceEntry> usb_device_entry;
    RequestPermissionCallback callback;

   private:
    DISALLOW_COPY_AND_ASSIGN(UsbPermissionRequest);
  };

  ~ArcUsbHostPermissionManager() override;
  static ArcUsbHostPermissionManager* GetForBrowserContext(
      content::BrowserContext* context);

  // ArcUsbHostUiDelegate override:
  void RequestUsbScanDeviceListPermission(
      const std::string& package_name,
      RequestPermissionCallback callback) override;
  void RequestUsbAccessPermission(const std::string& package_name,
                                  const std::string& guid,
                                  const base::string16& device_name,
                                  const base::string16& serial_number,
                                  uint16_t vendor_id,
                                  uint16_t product_id,
                                  RequestPermissionCallback callback) override;
  bool HasUsbAccessPermission(const std::string& package_name,
                              const std::string& guid,
                              const base::string16& serial_number,
                              uint16_t vendor_id,
                              uint16_t product_id) override;
  void DeviceRemoved(const std::string& guid) override;

  void ClearPermissionRequests() override;

  // ArcAppListPrefs::Observer override:
  void OnPackageRemoved(const std::string& package_name,
                        bool uninstalled) override;

  // Test method.
  std::vector<std::unique_ptr<UsbPermissionRequest>>&
  GetPendingRequestsForTest() {
    return pending_requests_;
  }

 private:
  friend class ArcUsbHostPermissionManagerFactory;

  ArcUsbHostPermissionManager(Profile* profile,
                              ArcAppListPrefs* arc_app_list_prefs,
                              ArcUsbHostBridge* arc_usb_host_bridge);

  static ArcUsbHostPermissionManager* Create(content::BrowserContext* context);

  // Restores Granted permissions. Called in constructor.
  void ReStorePermissionFromChromePrefs();

  // Tries to process next permission request as when UI is available. When
  // usb_permission_request in non-null, it is a new request from app. If
  // usb_permission_request is null. It is a request to excute remaining
  // requests as previous requests to UI is finished.
  void TryProcessNextPermissionRequest(
      std::unique_ptr<UsbPermissionRequest> usb_permission_request);

  bool HasUsbScanDeviceListPermission(const std::string& package_name) const;

  bool HasUsbAccessPermission(const std::string& package_name,
                              const UsbDeviceEntry* usb_device_entry);

  // Callback for UI permission dialog.
  void OnUsbPermissionReceived(bool accept);

  void TryUpdateArcUsbScanDeviceListPermission(const std::string& package_name,
                                               bool accept);

  void TryUpdateArcUsbAccessPermission(const std::string& package_name,
                                       const UsbDeviceEntry* usb_device_entry,
                                       bool accpet);

  std::vector<std::unique_ptr<UsbPermissionRequest>> pending_requests_;

  // Package permissions will be removed when package is uninstalled.
  // Dictory of package list that has been granted permission to scan
  // devicelist. It will be also stored in Chrome prefs.
  // We may need create UI to revoke this permission.
  std::unordered_set<std::string> usb_scan_devicelist_permission_dict_;

  // Package permissions will be removed when package is uninstalled.
  // Usb devices with serial numbers are considered as persistent devices.
  // Dictory of granted package to persistent devices access permission map.
  // It will also be stored in Chrome prefs.
  // We may need create UI to revoke this permission.
  std::unordered_map<std::string, std::vector<std::unique_ptr<UsbDeviceEntry>>>
      persistent_usb_access_permission_dict_;

  // Package permissions will be removed when package is uninstalled.
  // Dictory of granted package to ephemeral devices access permission map.
  // Permissions will be removed when device is detached or user session ends.
  std::unordered_map<std::string, std::vector<std::unique_ptr<UsbDeviceEntry>>>
      ephemeral_usb_access_permission_dict_;

  std::string current_requesting_package_;
  std::string current_requesting_guid_;
  bool ui_dialog_available_ = true;

  Profile* const profile_;

  ArcAppListPrefs* const arc_app_list_prefs_;

  ArcUsbHostBridge* const arc_usb_host_bridge_;

  base::WeakPtrFactory<ArcUsbHostPermissionManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcUsbHostPermissionManager);
};

}  // namespace arc

#endif  // CHROME_BROWSER_UI_APP_LIST_ARC_ARC_USB_HOST_PERMISSION_MANAGER_H_
