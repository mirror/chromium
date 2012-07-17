// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_DRM_H_
#define CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_DRM_H_
#pragma once

#include <set>

#include "base/basictypes.h"
#include "base/event_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
//#include "base/message_pump_epoll.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/display/output_configurator.h"

struct _drmModeCrtc;
typedef struct _drmModeCrtc drmModeCrtc;

namespace ui {
class OutputDRM;
class BufferDRM;
class ModeDRM;
}

namespace chromeos {

// This class interacts directly with the DRM API to manipulate outputd.
class CHROMEOS_EXPORT OutputConfiguratorDRM : public OutputConfigurator {
 public:
  OutputConfiguratorDRM();
  virtual ~OutputConfiguratorDRM();

  // Called when powerd notifies us that some set of displays should be turned
  // on or off.  This requires enabling or disabling the CRTC associated with
  // the display(s) in question so that the low power state is engaged.
  virtual bool ScreenPowerSet(bool power_on, bool all_displays);

  virtual bool SetDisplayMode(State new_state);

  virtual bool Dispatch(const base::NativeEvent& event) OVERRIDE;

 private:
  struct DisplayInfo {
    DisplayInfo(int fd) : fd(fd) {}
    int fd;
  };

  struct CachedOutputDescription {
    uint32_t output_id;
    uint32_t crtc_id;
    scoped_ptr<ui::ModeDRM> ideal_mode;
    scoped_ptr<ui::ModeDRM> mirror_mode;
    int x;
    int y;
    bool is_connected;
    bool is_powered_on;
    bool is_internal;
    unsigned long mm_width;
    unsigned long mm_height;
  };

  virtual bool TryRecacheOutputs(const DisplayInfo& info);

  virtual void UpdateCacheAndOutputToState(const DisplayInfo& info,
                                           State new_state);

  virtual bool RecacheAndUseDefaultState();

  virtual State InferCurrentState(const DisplayInfo& info) const;

  virtual bool IsProjecting() const;

  //MessagePumpEpollHandler* udev_handler_;
  //MessagePumpEpoll message_pump_;

  scoped_array<CachedOutputDescription> output_cache_;

  // The number of outputs in the output_cache_ array.
  int output_count_;
  
  std::set<ui::BufferDRM*> buffers_;

  DISALLOW_COPY_AND_ASSIGN(OutputConfiguratorDRM);
};

}  // namespace chromeos

#endif  // CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_DRM_H_
