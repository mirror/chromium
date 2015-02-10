// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/dri/native_display_delegate_dri.h"

#include "base/bind.h"
#include "ui/display/types/native_display_observer.h"
#include "ui/events/ozone/device/device_event.h"
#include "ui/ozone/platform/dri/display_mode_dri.h"
#include "ui/ozone/platform/dri/display_snapshot_dri.h"
#include "ui/ozone/platform/dri/dri_util.h"
#include "ui/ozone/platform/dri/dri_wrapper.h"
#include "ui/ozone/platform/dri/screen_manager.h"

namespace ui {

namespace {

const char kContentProtection[] = "Content Protection";

struct ContentProtectionMapping {
  const char* name;
  HDCPState state;
};

const ContentProtectionMapping kContentProtectionStates[] = {
    {"Undesired", HDCP_STATE_UNDESIRED},
    {"Desired", HDCP_STATE_DESIRED},
    {"Enabled", HDCP_STATE_ENABLED}};

uint32_t GetContentProtectionValue(drmModePropertyRes* property,
                                   HDCPState state) {
  std::string name;
  for (size_t i = 0; i < arraysize(kContentProtectionStates); ++i) {
    if (kContentProtectionStates[i].state == state) {
      name = kContentProtectionStates[i].name;
      break;
    }
  }

  for (int i = 0; i < property->count_enums; ++i)
    if (name == property->enums[i].name)
      return i;

  NOTREACHED();
  return 0;
}

class DisplaySnapshotComparator {
 public:
  explicit DisplaySnapshotComparator(const DisplaySnapshotDri* snapshot)
      : crtc_(snapshot->crtc()), connector_(snapshot->connector()) {}

  DisplaySnapshotComparator(uint32_t crtc, uint32_t connector)
      : crtc_(crtc), connector_(connector) {}

  bool operator()(const DisplaySnapshotDri* other) const {
    return connector_ == other->connector() && crtc_ == other->crtc();
  }

 private:
  uint32_t crtc_;
  uint32_t connector_;
};

}  // namespace

NativeDisplayDelegateDri::NativeDisplayDelegateDri(
    DriWrapper* dri,
    ScreenManager* screen_manager)
    : dri_(dri), screen_manager_(screen_manager) {
  // TODO(dnicoara): Remove when async display configuration is supported.
  screen_manager_->ForceInitializationOfPrimaryDisplay();
}

NativeDisplayDelegateDri::~NativeDisplayDelegateDri() {
}

DisplaySnapshot* NativeDisplayDelegateDri::FindDisplaySnapshot(int64_t id) {
  for (size_t i = 0; i < cached_displays_.size(); ++i)
    if (cached_displays_[i]->display_id() == id)
      return cached_displays_[i];

  return NULL;
}

const DisplayMode* NativeDisplayDelegateDri::FindDisplayMode(
    const gfx::Size& size,
    bool is_interlaced,
    float refresh_rate) {
  for (size_t i = 0; i < cached_modes_.size(); ++i)
    if (cached_modes_[i]->size() == size &&
        cached_modes_[i]->is_interlaced() == is_interlaced &&
        cached_modes_[i]->refresh_rate() == refresh_rate)
      return cached_modes_[i];

  return NULL;
}

void NativeDisplayDelegateDri::Initialize() {
}

void NativeDisplayDelegateDri::GrabServer() {
}

void NativeDisplayDelegateDri::UngrabServer() {
}

bool NativeDisplayDelegateDri::TakeDisplayControl() {
  if (!dri_->SetMaster()) {
    LOG(ERROR) << "Failed to take control of the display";
    return false;
  }
  return true;
}

bool NativeDisplayDelegateDri::RelinquishDisplayControl() {
  if (!dri_->DropMaster()) {
    LOG(ERROR) << "Failed to relinquish control of the display";
    return false;
  }
  return true;
}

void NativeDisplayDelegateDri::SyncWithServer() {
}

void NativeDisplayDelegateDri::SetBackgroundColor(uint32_t color_argb) {
}

void NativeDisplayDelegateDri::ForceDPMSOn() {
  for (size_t i = 0; i < cached_displays_.size(); ++i) {
    DisplaySnapshotDri* dri_output = cached_displays_[i];
    if (dri_output->dpms_property())
      dri_->SetProperty(dri_output->connector(),
                        dri_output->dpms_property()->prop_id, DRM_MODE_DPMS_ON);
  }
}

std::vector<DisplaySnapshot*> NativeDisplayDelegateDri::GetDisplays() {
  ScopedVector<DisplaySnapshotDri> old_displays(cached_displays_.Pass());
  ScopedVector<const DisplayMode> old_modes(cached_modes_.Pass());

  ScopedVector<HardwareDisplayControllerInfo> displays =
      GetAvailableDisplayControllerInfos(dri_->get_fd());
  for (size_t i = 0; i < displays.size(); ++i) {
    DisplaySnapshotDri* display = new DisplaySnapshotDri(
        dri_, displays[i]->connector(), displays[i]->crtc(), i);

    // If the display exists make sure to sync up the new snapshot with the old
    // one to keep the user configured details.
    auto it = std::find_if(
        old_displays.begin(), old_displays.end(),
        DisplaySnapshotComparator(displays[i]->crtc()->crtc_id,
                                  displays[i]->connector()->connector_id));
    // Origin is only used within the platform code to keep track of the display
    // location.
    if (it != old_displays.end())
      display->set_origin((*it)->origin());

    cached_displays_.push_back(display);
    cached_modes_.insert(cached_modes_.end(), display->modes().begin(),
                         display->modes().end());
  }

  NotifyScreenManager(cached_displays_.get(), old_displays.get());

  std::vector<DisplaySnapshot*> generic_displays(cached_displays_.begin(),
                                                 cached_displays_.end());
  return generic_displays;
}

void NativeDisplayDelegateDri::GetDisplays(
    const GetDisplaysCallback& callback) {
  NOTREACHED();
}

void NativeDisplayDelegateDri::AddMode(const DisplaySnapshot& output,
                                       const DisplayMode* mode) {
}

bool NativeDisplayDelegateDri::Configure(const DisplaySnapshot& output,
                                         const DisplayMode* mode,
                                         const gfx::Point& origin) {
  const DisplaySnapshotDri& dri_output =
      static_cast<const DisplaySnapshotDri&>(output);

  VLOG(1) << "DRM configuring: crtc=" << dri_output.crtc()
          << " connector=" << dri_output.connector()
          << " origin=" << origin.ToString()
          << " size=" << (mode ? mode->size().ToString() : "0x0");

  if (mode) {
    if (!screen_manager_->ConfigureDisplayController(
            dri_output.crtc(), dri_output.connector(), origin,
            static_cast<const DisplayModeDri*>(mode)->mode_info())) {
      VLOG(1) << "Failed to configure: crtc=" << dri_output.crtc()
              << " connector=" << dri_output.connector();
      return false;
    }
  } else {
    if (dri_output.dpms_property()) {
      dri_->SetProperty(dri_output.connector(),
                        dri_output.dpms_property()->prop_id, DRM_MODE_DPMS_OFF);
    }

    if (!screen_manager_->DisableDisplayController(dri_output.crtc())) {
      VLOG(1) << "Failed to disable crtc=" << dri_output.crtc();
      return false;
    }
  }

  return true;
}

void NativeDisplayDelegateDri::Configure(const DisplaySnapshot& output,
                                         const DisplayMode* mode,
                                         const gfx::Point& origin,
                                         const ConfigureCallback& callback) {
  NOTREACHED();
}

void NativeDisplayDelegateDri::CreateFrameBuffer(const gfx::Size& size) {
}

bool NativeDisplayDelegateDri::GetHDCPState(const DisplaySnapshot& output,
                                            HDCPState* state) {
  const DisplaySnapshotDri& dri_output =
      static_cast<const DisplaySnapshotDri&>(output);

  ScopedDrmConnectorPtr connector(dri_->GetConnector(dri_output.connector()));
  if (!connector) {
    LOG(ERROR) << "Failed to get connector " << dri_output.connector();
    return false;
  }

  ScopedDrmPropertyPtr hdcp_property(
      dri_->GetProperty(connector.get(), kContentProtection));
  if (!hdcp_property) {
    LOG(ERROR) << "'" << kContentProtection << "' property doesn't exist.";
    return false;
  }

  DCHECK_LT(static_cast<int>(hdcp_property->prop_id), connector->count_props);
  int hdcp_state_idx = connector->prop_values[hdcp_property->prop_id];
  DCHECK_LT(hdcp_state_idx, hdcp_property->count_enums);

  std::string name(hdcp_property->enums[hdcp_state_idx].name);
  for (size_t i = 0; i < arraysize(kContentProtectionStates); ++i) {
    if (name == kContentProtectionStates[i].name) {
      *state = kContentProtectionStates[i].state;
      VLOG(3) << "HDCP state: " << *state << " (" << name << ")";
      return true;
    }
  }

  LOG(ERROR) << "Unknown content protection value '" << name << "'";
  return false;
}

bool NativeDisplayDelegateDri::SetHDCPState(const DisplaySnapshot& output,
                                            HDCPState state) {
  const DisplaySnapshotDri& dri_output =
      static_cast<const DisplaySnapshotDri&>(output);

  ScopedDrmConnectorPtr connector(dri_->GetConnector(dri_output.connector()));
  if (!connector) {
    LOG(ERROR) << "Failed to get connector " << dri_output.connector();
    return false;
  }

  ScopedDrmPropertyPtr hdcp_property(
      dri_->GetProperty(connector.get(), kContentProtection));
  if (!hdcp_property) {
    LOG(ERROR) << "'" << kContentProtection << "' property doesn't exist.";
    return false;
  }

  return dri_->SetProperty(
      dri_output.connector(), hdcp_property->prop_id,
      GetContentProtectionValue(hdcp_property.get(), state));
}

std::vector<ui::ColorCalibrationProfile>
NativeDisplayDelegateDri::GetAvailableColorCalibrationProfiles(
    const ui::DisplaySnapshot& output) {
  NOTIMPLEMENTED();
  return std::vector<ui::ColorCalibrationProfile>();
}

bool NativeDisplayDelegateDri::SetColorCalibrationProfile(
    const ui::DisplaySnapshot& output,
    ui::ColorCalibrationProfile new_profile) {
  NOTIMPLEMENTED();
  return false;
}

void NativeDisplayDelegateDri::AddObserver(NativeDisplayObserver* observer) {
  observers_.AddObserver(observer);
}

void NativeDisplayDelegateDri::RemoveObserver(NativeDisplayObserver* observer) {
  observers_.RemoveObserver(observer);
}

void NativeDisplayDelegateDri::NotifyScreenManager(
    const std::vector<DisplaySnapshotDri*>& new_displays,
    const std::vector<DisplaySnapshotDri*>& old_displays) const {
  for (size_t i = 0; i < old_displays.size(); ++i) {
    const std::vector<DisplaySnapshotDri*>::const_iterator it =
        std::find_if(new_displays.begin(), new_displays.end(),
                     DisplaySnapshotComparator(old_displays[i]));

    if (it == new_displays.end())
      screen_manager_->RemoveDisplayController(old_displays[i]->crtc());
  }

  for (size_t i = 0; i < new_displays.size(); ++i) {
    const std::vector<DisplaySnapshotDri*>::const_iterator it =
        std::find_if(old_displays.begin(), old_displays.end(),
                     DisplaySnapshotComparator(new_displays[i]));

    if (it == old_displays.end())
      screen_manager_->AddDisplayController(dri_, new_displays[i]->crtc(),
                                            new_displays[i]->connector());
  }
}

}  // namespace ui
