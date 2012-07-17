// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/display/output_configurator_drm.h"

#include <xf86drmMode.h>
#include <map>

#include "base/chromeos/chromeos_version.h"
#include "base/logging.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "dbus/bus.h"
#include "dbus/exported_object.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/aura/root_window_host_drm.h"
#include "ui/base/output_drm.h"

namespace aura {
class CompositorLock;
}  // namespace aura

namespace {

const float kMmInInch = 25.4;
// The DPI threshold to detech high density screen.
// Higher DPI than this will use device_scale_factor=2
// Should be kept in sync with monitor_change_observer_x11.cc
const unsigned int kHighDensityDIPThreshold = 160;

// Gap between screens so cursor at bottom of active monitor doesn't partially
// appear on top of inactive monitor. Higher numbers guard against larger
// cursors, but also waste more memory. We will double this gap for screens
// with a device_scale_factor of 2. While this gap will not guard against all
// possible cursors in X, it should handle the ones we actually use. See
// crbug.com/130188
const int kVerticalGap = 30;

// A helper to determine the device_scale_factor given pixel width and mm_width.
// This currently only reports two scale factors (1.0 and 2.0)
static float ComputeDeviceScaleFactor(unsigned int width,
                                      unsigned long mm_width) {
  float device_scale_factor = 1.0f;
  if (mm_width > 0 && (kMmInInch * width / mm_width) > kHighDensityDIPThreshold)
    device_scale_factor = 2.0f;
  return device_scale_factor;
}

bool FindMirrorModeForOutputs(const ui::OutputDRM& output_one,
                              const ui::OutputDRM& output_two,
                              ui::ModeDRM* mode_one,
                              ui::ModeDRM* mode_two) {
  std::vector<ui::ModeDRM> one_modes;
  std::vector<ui::ModeDRM> two_modes;
  output_one.GetModes(&one_modes);
  output_two.GetModes(&two_modes);
  unsigned int one_index = 0;
  unsigned int two_index = 0;
  while ((one_index < one_modes.size()) && (two_index < two_modes.size())) {
    const ui::ModeDRM& one_mode = one_modes[one_index];
    const ui::ModeDRM& two_mode = two_modes[two_index];
    int one_width = one_mode.width();
    int one_height = one_mode.height();
    int two_width = two_mode.width();
    int two_height = two_mode.height();
    if ((one_mode.width() == two_mode.width()) &&
        (one_mode.height() == two_mode.height())) {
      *mode_one = one_mode;
      *mode_two = two_mode;
      return true;
    } else {
      // The sort order of the modes is NOT by mode area but is sorted by width,
      // then by height within each like width.
      if (one_width > two_width) {
        one_index += 1;
      } else if (one_width < two_width) {
        two_index += 1;
      } else {
        if (one_height > two_height) {
          one_index += 1;
        } else {
          two_index += 1;
        }
      }
    }
  }
  return false;
}

void ConfigureCrtc(int fd, uint32_t crtc_id, int x, int y, uint32_t fb_id,
                   const ui::ModeDRM& mode, uint32_t output_id) {
  aura::RootWindowHost* window_host =
      aura::RootWindowHostDRM::Create(gfx::Rect(mode.width(), mode.height()));
  aura::RootWindowHostDRM* window_host_drm =
      reinterpret_cast<aura::RootWindowHostDRM*>(window_host);

  // TODO(sque): use these features?
  //uint32_t crtc_id = window_host->GetAcceleratedWidget();
  //scoped_refptr<aura::CompositorLock> lock =
  //    window_host->GetRootWindow()->GetCompositorLock();

  drmModeCrtc new_crtc;
  ui::DRMGetCRTC(fd, crtc_id, &new_crtc);
  new_crtc.x = x;
  new_crtc.y = y;
  new_crtc.buffer_id = fb_id;
  new_crtc.mode = mode.mode_info();

  ui::OutputDRM output(fd, output_id);
  output.SetCRTC(new_crtc);

  // TODO(sque): don't use this hack.
  window_host_drm->SetSize(gfx::Size(mode.width(), mode.height()));
}

// TODO(sque): make buffer management more localized.
std::map<uint32_t, ui::BufferDRM*> g_buffers;

uint32_t CreateFrameBuffer(int fd, uint32_t width, uint32_t height) {
  ui::BufferDRM *buffer = new ui::BufferDRM(fd, width, height);
  g_buffers[buffer->GetBufferID()] = buffer;
  return buffer->GetBufferID();
}

void GetOutputConfiguration(int fd,
                            int connector_id,
                            drmModeCrtc* crtc,
                            ui::ModeDRM* mode,
                            int* height) {
  ui::OutputDRM output(fd, connector_id);
  output.GetCRTC(crtc);
  *mode = crtc->mode;
  *height = output.GetPreferredMode().height();
}

}  // namespace

namespace chromeos {

OutputConfiguratorDRM::OutputConfiguratorDRM() : output_count_(0) {
  if (base::chromeos::IsRunningOnChromeOS()) {
    // Send the signal to powerd to tell it that we will take over output
    // control.
    // Note that this can be removed once the legacy powerd support is removed.
    chromeos::DBusThreadManager* manager = chromeos::DBusThreadManager::Get();
    dbus::Bus* bus = manager->GetSystemBus();
    dbus::ExportedObject* remote_object = bus->GetExportedObject(
        dbus::ObjectPath(power_manager::kPowerManagerServicePath));
    dbus::Signal signal(power_manager::kPowerManagerInterface,
                        power_manager::kUseNewMonitorConfigSignal);
    CHECK(signal.raw_message() != NULL);
    remote_object->SendSignal(&signal);
    int fd = aura::RootWindowHostDRM::GetDRMFd();
    // Cache the initial output state.
    if (!TryRecacheOutputs(fd)) {
      // Make sure the system has outputs at all (connected or not).
      std::vector<ui::OutputDRM*> all_outputs;
      CHECK(ui::DRMGetNumOutputs(fd) > 0);
    }
    State current_state = InferCurrentState(fd);
    if (current_state == STATE_INVALID) {
      // Unknown state.  Transition into the default state.
      State state = GetDefaultState();
      UpdateCacheAndOutputToState(fd, state);
    } else {
      // This is a valid state so just save it to |output_state_|.
      output_state_ = current_state;
    }
    CheckIsProjectingAndNotify();
  }
}

OutputConfiguratorDRM::~OutputConfiguratorDRM() {}

void OutputConfiguratorDRM::UpdateCacheAndOutputToState(const DisplayInfo& info,
                                                        State new_state) {
  int fd = info.fd;

  // First, calculate the width and height of the framebuffer (we could retain
  // the existing buffer, if it isn't resizing, but that causes an odd display
  // state where the CRTCs are repositioned over the root windows before Chrome
  // can move them).  It is a feature worth considering, though, and wouldn't
  // be difficult to implement (just check the current framebuffer size before
  // changing it).
  int width = 0;
  int height = 0;
  int primary_height = 0;
  int secondary_height = 0;
  int vertical_gap = 0;
  if (new_state == STATE_SINGLE) {
    CHECK_NE(-1, primary_output_index_);
    ui::ModeDRM ideal_mode = *output_cache_[primary_output_index_].ideal_mode;
    width = ideal_mode.width();
    height = ideal_mode.height();
  } else if (new_state == STATE_DUAL_MIRROR) {
    CHECK_NE(-1, primary_output_index_);
    CHECK_NE(-1, secondary_output_index_);
    ui::ModeDRM mirror_mode = *output_cache_[primary_output_index_].mirror_mode;
    width = mirror_mode.width();
    height = mirror_mode.height();
  } else if ((new_state == STATE_DUAL_PRIMARY_ONLY) ||
             (new_state == STATE_DUAL_SECONDARY_ONLY)) {
    CHECK_NE(-1, primary_output_index_);
    CHECK_NE(-1, secondary_output_index_);

    ui::ModeDRM one_ideal = *output_cache_[primary_output_index_].ideal_mode;
    ui::ModeDRM two_ideal = *output_cache_[secondary_output_index_].ideal_mode;

    // Compute the device scale factor for the topmost display. We only need
    // to take this device's scale factor into account as we are creating a gap
    // to avoid the cursor drawing onto the second (unused) display when the
    // cursor is near the bottom of the topmost display.
    float top_scale_factor;
    if (new_state == STATE_DUAL_PRIMARY_ONLY) {
      top_scale_factor = ComputeDeviceScaleFactor(one_ideal.width(),
          output_cache_[primary_output_index_].mm_width);
    } else {
      top_scale_factor = ComputeDeviceScaleFactor(two_ideal.width(),
          output_cache_[secondary_output_index_].mm_width);
    }
    vertical_gap = kVerticalGap * top_scale_factor;
    width = std::max<int>(one_ideal.width(), two_ideal.width());
    height = one_ideal.height() + two_ideal.height() + vertical_gap;
    primary_height = one_ideal.height();
    secondary_height = two_ideal.height();
  }
  uint32_t fb_id = CreateFrameBuffer(fd, width, height);

  // Now, tile the outputs appropriately.
  const int x = 0;
  const int y = 0;
  switch (new_state) {
    case STATE_SINGLE:
      ConfigureCrtc(fd,
                    output_cache_[primary_output_index_].crtc_id,
                    x,
                    y,
                    fb_id,
                    *output_cache_[primary_output_index_].ideal_mode,
                    output_cache_[primary_output_index_].output_id);
      break;
    case STATE_DUAL_MIRROR:
    case STATE_DUAL_PRIMARY_ONLY:
    case STATE_DUAL_SECONDARY_ONLY: {
      ui::ModeDRM primary_mode =
          *output_cache_[primary_output_index_].mirror_mode;
      ui::ModeDRM secondary_mode =
          *output_cache_[secondary_output_index_].mirror_mode;
      int primary_y = y;
      int secondary_y = y;

      if (new_state != STATE_DUAL_MIRROR) {
        primary_mode = *output_cache_[primary_output_index_].ideal_mode;
        secondary_mode = *output_cache_[secondary_output_index_].ideal_mode;
      }
      if (new_state == STATE_DUAL_PRIMARY_ONLY)
        secondary_y = y + primary_height + vertical_gap;
      if (new_state == STATE_DUAL_SECONDARY_ONLY)
        primary_y = y + secondary_height + vertical_gap;

      ConfigureCrtc(fd,
                    output_cache_[primary_output_index_].crtc_id,
                    x,
                    primary_y,
                    fb_id,
                    primary_mode,
                    output_cache_[primary_output_index_].output_id);
      ConfigureCrtc(fd,
                    output_cache_[secondary_output_index_].crtc_id,
                    x, 
                    secondary_y,
                    fb_id,
                    secondary_mode,
                    output_cache_[secondary_output_index_].output_id);
      }
      break;
    case STATE_HEADLESS:
      // Do nothing.
      break;
    default:
      NOTREACHED() << "Unhandled state " << new_state;
  }
  output_state_ = new_state;
}

bool OutputConfiguratorDRM::ScreenPowerSet(bool power_on, bool all_displays) {
  // TODO(sque)
  NOTIMPLEMENTED();
  return true;
}

bool OutputConfiguratorDRM::SetDisplayMode(State new_state) {
  if (output_state_ == STATE_INVALID ||
      output_state_ == STATE_HEADLESS ||
      output_state_ == STATE_SINGLE)
    return false;

  UpdateCacheAndOutputToState(aura::RootWindowHostDRM::GetDRMFd(), new_state);
  return true;
}

bool OutputConfiguratorDRM::Dispatch(const base::NativeEvent& event) {
  RecacheAndUseDefaultState();
  CheckIsProjectingAndNotify();
  return true;
}

bool OutputConfiguratorDRM::TryRecacheOutputs(const DisplayInfo& info) {
  bool outputs_did_change = false;
  int previous_connected_count = 0;
  int new_connected_count = 0;
  int fd = info.fd;

  if (output_count_ != ui::DRMGetNumOutputs(fd)) {
    outputs_did_change = true;
  } else {
    // The outputs might have changed so compare the connected states in the
    // screen to our existing cache.
    std::vector<uint32_t> outputs;
    ui::DRMGetAllOutputs(fd, &outputs);
    for (int i = 0; (i < output_count_) && !outputs_did_change; ++i) {
      //uint32_t current_id = outputs[i].GetID();
      //XRROutputInfo* output = XRRGetOutputInfo(display, screen, thisID);
      CHECK(i < static_cast<int>(outputs.size()));
      const ui::OutputDRM output(fd, outputs[i]);
      bool now_connected = output.IsConnected();
      outputs_did_change = (now_connected != output_cache_[i].is_connected);

      if (output_cache_[i].is_connected)
        previous_connected_count += 1;
      if (now_connected)
        new_connected_count += 1;
    }
  }

  if (outputs_did_change) {
    // We now know that we need to recache so free and re-alloc the buffer.
    output_count_ = ui::DRMGetNumOutputs(fd);
    if (output_count_ == 0) {
      output_cache_.reset(NULL);
    } else {
      // Ideally, this would be allocated inline in the OutputConfiguratorDRM
      // instance since we support at most 2 connected outputs but this dynamic
      // allocation was specifically requested.
      output_cache_.reset(new CachedOutputDescription[output_count_]);
    }

    // TODO: This approach to finding CRTCs only supports two.  Expand on this.
    drmModeCrtc used_crtc;
    memset(&used_crtc, 0, sizeof(used_crtc));
    primary_output_index_ = -1;
    secondary_output_index_ = -1;

    std::vector<uint32_t> outputs;
    ui::DRMGetAllOutputs(fd, &outputs);
    for (int i = 0; i < output_count_; ++i) {
      const ui::OutputDRM output(fd, outputs[i]);
      bool is_connected = output.IsConnected();
      drmModeCrtc crtc;
      memset(&crtc, 0, sizeof(drmModeCrtc));
      ui::ModeDRM ideal_mode;
      int x = 0;
      int y = 0;
      //unsigned long mm_width = output->mm_width;
      //unsigned long mm_height = output->mm_height;
      unsigned long mm_width = 0;
      unsigned long mm_height = 0;
      bool is_internal = false;

      bool crtc_found = false;
      if (is_connected) {
        std::vector<drmModeCrtc> crtcs;
        output.GetCRTCs(&crtcs);
        int num_crtcs = crtcs.size();
        for (int j = 0; (j < num_crtcs) && !crtc_found; ++j) {
          const drmModeCrtc& possible = crtcs[j];
          if (possible.crtc_id != used_crtc.crtc_id) {
            crtc = possible;
            used_crtc = possible;
            crtc_found = true;
          }
        }
        
        is_internal = output.IsInternal();
        ideal_mode = output.GetPreferredMode();
        if (crtc_found) {
          x = crtc.x;
          y = crtc.y;
        }
        // Save this for later mirror mode detection.
        if (primary_output_index_ == -1)
          primary_output_index_ = i;
        else if (secondary_output_index_ == -1)
          secondary_output_index_ = i;
      }
      output_cache_[i].output_id = output.GetID();
      output_cache_[i].crtc_id = crtc.crtc_id;
      output_cache_[i].mirror_mode.reset(NULL);
      output_cache_[i].ideal_mode.reset(new ui::ModeDRM(ideal_mode));
      output_cache_[i].x = x;
      output_cache_[i].y = y;
      output_cache_[i].is_connected = is_connected;
      output_cache_[i].is_powered_on = true;
      output_cache_[i].is_internal = is_internal;
      output_cache_[i].mm_width = mm_width;
      output_cache_[i].mm_height = mm_height;
    }
    // Now, detect the mirror modes if we have two connected outputs.
    if ((primary_output_index_ != -1) && (secondary_output_index_ != -1)) {
      mirror_supported_ = FindMirrorModeForOutputs(
          ui::OutputDRM(fd, output_cache_[primary_output_index_].output_id),
          ui::OutputDRM(fd, output_cache_[secondary_output_index_].output_id),
          output_cache_[primary_output_index_].mirror_mode.get(),
          output_cache_[secondary_output_index_].mirror_mode.get());
    }
  }
  return outputs_did_change;
}

bool OutputConfiguratorDRM::RecacheAndUseDefaultState() {
  if (!TryRecacheOutputs(aura::RootWindowHostDRM::GetDRMFd()))
    return false;
  State state = GetDefaultState();
  UpdateCacheAndOutputToState(aura::RootWindowHostDRM::GetDRMFd(), state);
  return true;
}

State OutputConfiguratorDRM::InferCurrentState(const DisplayInfo& info) const {
  // STATE_INVALID will be our default or "unknown" state.
  State state = STATE_INVALID;

  int fd = info.fd;

  // First step:  count the number of connected outputs.
  if (secondary_output_index_ == -1) {
    // No secondary display.
    if (primary_output_index_ == -1) {
      // No primary display implies HEADLESS.
      state = STATE_HEADLESS;
    } else {
      // The common case of primary-only.
      // The only sanity check we require in this case is that the current mode
      // of the output's CRTC is the ideal mode we determined for it.
      drmModeCrtc primary_crtc;
      ui::ModeDRM primary_mode;
      int primary_height = 0;
      GetOutputConfiguration(fd,
                             output_cache_[primary_output_index_].output_id,
                             &primary_crtc,
                             &primary_mode,
                             &primary_height);
      if (primary_mode == *output_cache_[primary_output_index_].ideal_mode)
        state = STATE_SINGLE;
    }
  } else {
    // We have two displays attached so we need to look at their configuration.
    // Note that, for simplicity, we will only detect the states that we would
    // have used and will assume anything unexpected is INVALID (which should
    // not happen in any expected usage scenario).
    drmModeCrtc primary_crtc;
    ui::ModeDRM primary_mode;
    int primary_height = 0;
    GetOutputConfiguration(fd,
                           output_cache_[primary_output_index_].output_id,
                           &primary_crtc,
                           &primary_mode,
                           &primary_height);

    drmModeCrtc secondary_crtc;
    ui::ModeDRM secondary_mode;
    int secondary_height = 0;
    GetOutputConfiguration(fd,
                           output_cache_[secondary_output_index_].output_id,
                           &secondary_crtc,
                           &secondary_mode,
                           &secondary_height);

    // Make sure the CRTCs are matched to the expected outputs.
    if ((output_cache_[primary_output_index_].crtc_id ==
            primary_crtc.crtc_id) &&
        (output_cache_[secondary_output_index_].crtc_id ==
            secondary_crtc.crtc_id)) {
      // Check the mode matching:  either both mirror or both ideal.
      if ((*output_cache_[primary_output_index_].mirror_mode ==
              primary_mode) &&
          (*output_cache_[secondary_output_index_].mirror_mode ==
              secondary_mode)) {
        // We are already in mirror mode.
        state = STATE_DUAL_MIRROR;
      } else if ((*output_cache_[primary_output_index_].ideal_mode ==
                     primary_mode) &&
                 (*output_cache_[secondary_output_index_].ideal_mode ==
                     secondary_mode)) {
        // Both outputs are in their "ideal" mode so check their Y-offsets to
        // see which "ideal" configuration this is.
        if (primary_height == output_cache_[secondary_output_index_].y) {
          // Secondary is tiled first.
          state = STATE_DUAL_SECONDARY_ONLY;
        } else if (secondary_height == output_cache_[primary_output_index_].y) {
          // Primary is tiled first.
          state = STATE_DUAL_PRIMARY_ONLY;
        }
      }
    }
  }
  return state;
}

bool OutputConfiguratorDRM::IsProjecting() const {
  // Determine if there is an "internal" output and how many outputs are
  // connected.
  bool has_internal_output = false;
  int connected_output_count = 0;
  for (int i = 0; i < output_count_; ++i) {
    if (output_cache_[i].is_connected) {
      connected_output_count += 1;
      has_internal_output |= output_cache_[i].is_internal;
    }
  }

  // "Projecting" is defined as having more than 1 output connected while at
  // least one of them is an internal output.
  return has_internal_output && (connected_output_count > 1);
}

}  // namespace chromeos
