// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_NATIVE_DISPLAY_DELEGATE_HOST_H_
#define UI_OZONE_PLATFORM_DRM_HOST_NATIVE_DISPLAY_DELEGATE_HOST_H_

#include <map>
#include <queue>

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_vector.h"
#include "base/observer_list.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/events/ozone/device/device_event.h"
#include "ui/events/ozone/device/device_event_observer.h"
#include "ui/ozone/public/gpu_platform_support_host.h"

namespace ui {

class DeviceManager;
class DisplayManager;
class DrmDeviceHandle;
class DrmGpuPlatformSupportHost;

struct DisplaySnapshot_Params;

class DrmNativeDisplayDelegate : public NativeDisplayDelegate,
                                 public DeviceEventObserver,
                                 public GpuPlatformSupportHost {
 public:
  DrmNativeDisplayDelegate(DrmGpuPlatformSupportHost* proxy,
                           DeviceManager* device_manager,
                           DisplayManager* display_manager,
                           const base::FilePath& primary_graphics_card_path);
  ~DrmNativeDisplayDelegate() override;

  // NativeDisplayDelegate overrides:
  void Initialize() override;
  void GrabServer() override;
  void UngrabServer() override;
  bool TakeDisplayControl() override;
  bool RelinquishDisplayControl() override;
  void SyncWithServer() override;
  void SetBackgroundColor(uint32_t color_argb) override;
  void ForceDPMSOn() override;
  void GetDisplays(const GetDisplaysCallback& callback) override;
  void AddMode(const DisplaySnapshot& output, const DisplayMode* mode) override;
  void Configure(const DisplaySnapshot& output,
                 const DisplayMode* mode,
                 const gfx::Point& origin,
                 const ConfigureCallback& callback) override;
  void CreateFrameBuffer(const gfx::Size& size) override;
  void GetHDCPState(const DisplaySnapshot& output,
                    const GetHDCPStateCallback& callback) override;
  void SetHDCPState(const DisplaySnapshot& output,
                    HDCPState state,
                    const SetHDCPStateCallback& callback) override;
  std::vector<ColorCalibrationProfile> GetAvailableColorCalibrationProfiles(
      const DisplaySnapshot& output) override;
  bool SetColorCalibrationProfile(const DisplaySnapshot& output,
                                  ColorCalibrationProfile new_profile) override;
  void AddObserver(NativeDisplayObserver* observer) override;
  void RemoveObserver(NativeDisplayObserver* observer) override;

  // DeviceEventObserver overrides:
  void OnDeviceEvent(const DeviceEvent& event) override;

  // GpuPlatformSupportHost:
  void OnChannelEstablished(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& send_callback) override;
  void OnChannelDestroyed(int host_id) override;

  // IPC::Listener overrides:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  struct DisplayEvent {
    DisplayEvent(DeviceEvent::ActionType action_type,
                 const base::FilePath& path)
        : action_type(action_type), path(path) {}

    DeviceEvent::ActionType action_type;
    base::FilePath path;
  };

  void OnUpdateNativeDisplays(
      const std::vector<DisplaySnapshot_Params>& displays);
  void OnDisplayConfigured(int64_t display_id, bool status);

  void ProcessEvent();

  // Called as a result of finishing to process the display hotplug event. These
  // are responsible for dequing the event and scheduling the next event.
  void OnAddGraphicsDevice(const base::FilePath& path,
                           scoped_ptr<DrmDeviceHandle> handle);
  void OnUpdateGraphicsDevice();
  void OnRemoveGraphicsDevice(const base::FilePath& path);

  void OnHDCPStateReceived(int64_t display_id, bool status, HDCPState state);
  void OnHDCPStateUpdated(int64_t display_id, bool status);

  void RunUpdateDisplaysCallback(const GetDisplaysCallback& callback) const;

  DrmGpuPlatformSupportHost* proxy_;  // Not owned.
  DeviceManager* device_manager_;     // Not owned.
  DisplayManager* display_manager_;   // Not owned.

  // File path for the primary graphics card which is opened by default in the
  // GPU process. We'll avoid opening this in hotplug events since it will race
  // with the GPU process trying to open it and aquire DRM master.
  const base::FilePath primary_graphics_card_path_;

  // Keeps track if there is a dummy display. This happens on initialization
  // when there is no connection to the GPU to update the displays.
  bool has_dummy_display_;

  ScopedVector<DisplaySnapshot> displays_;
  ObserverList<NativeDisplayObserver> observers_;

  GetDisplaysCallback get_displays_callback_;

  // Map between display_id and the configuration callback.
  std::map<int64_t, ConfigureCallback> configure_callback_map_;

  std::map<int64_t, GetHDCPStateCallback> get_hdcp_state_callback_map_;

  std::map<int64_t, SetHDCPStateCallback> set_hdcp_state_callback_map_;

  // Used to serialize display event processing. This is done since
  // opening/closing DRM devices cannot be done on the UI thread and are handled
  // on a worker thread. Thus, we need to queue events in order to process them
  // in the correct order.
  std::queue<DisplayEvent> event_queue_;

  // True if a display event is currently being processed on a worker thread.
  bool task_pending_;

  // Keeps track of all the active DRM devices.
  base::ScopedPtrHashMap<base::FilePath, scoped_ptr<DrmDeviceHandle>>
      drm_devices_;

  base::WeakPtrFactory<DrmNativeDisplayDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DrmNativeDisplayDelegate);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_HOST_NATIVE_DISPLAY_DELEGATE_HOST_H_
