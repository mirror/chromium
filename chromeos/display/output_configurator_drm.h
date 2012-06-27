// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_DRM_H_
#define CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_DRM_H_
#pragma once

#include "base/basictypes.h"
#include "base/event_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/display/output_configurator.h"

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

  virtual bool TryRecacheOutputs(const DisplayInfo& info);

  virtual void UpdateCacheAndOutputToState(const DisplayInfo& info,
                                           State new_state);

  virtual bool RecacheAndUseDefaultState();

  virtual State InferCurrentState(const DisplayInfo& info) const;

  virtual bool IsProjecting() const;

  DISALLOW_COPY_AND_ASSIGN(OutputConfiguratorDRM);
};

}  // namespace chromeos

#endif  // CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_DRM_H_
