// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_X11_H_
#define CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_X11_H_
#pragma once

#include "base/basictypes.h"
#include "base/event_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/display/output_configurator.h"

// Forward declarations for Xlib and Xrandr.
typedef unsigned long XID;
typedef XID Window;
typedef XID RROutput;
typedef XID RRCrtc;
typedef XID RRMode;

struct _XRRScreenResources;
typedef _XRRScreenResources XRRScreenResources;

namespace chromeos {

// This class interacts directly with the underlying Xrandr API to manipulate
// CTRCs and Outputs.  It will likely grow more state, over time, or expose
// Output info in other ways as more of the Chrome display code grows up around
// it.
class CHROMEOS_EXPORT OutputConfiguratorX11 : public OutputConfigurator {
 public:
  OutputConfiguratorX11();
  virtual ~OutputConfiguratorX11();

  // Called when powerd notifies us that some set of displays should be turned
  // on or off.  This requires enabling or disabling the CRTC associated with
  // the display(s) in question so that the low power state is engaged.
  virtual bool ScreenPowerSet(bool power_on, bool all_displays) OVERRIDE;

  virtual bool SetDisplayMode(State new_state) OVERRIDE;

  // Called when an RRNotify event is received.  The implementation is
  // interested in the cases of RRNotify events which correspond to output
  // add/remove events.  Note that Output add/remove events are sent in response
  // to our own reconfiguration operations so spurious events are common.
  // Spurious events will have no effect.
  virtual bool Dispatch(const base::NativeEvent& event) OVERRIDE;

 private:
  struct DisplayInfo {
    Display* display;
    XRRScreenResources* screen;
    DisplayInfo(Display* display, XRRScreenResources* screen)
        : display(display), screen(screen) {}
  };

  // The information we need to cache from an output to implement operations
  // such as power state but also to eliminate duplicate operations within a
  // given action (determining which CRTC to use for a given output, for
  // example).
  struct CachedOutputDescription {
    RROutput output;
    RRCrtc crtc;
    RRMode mirror_mode;
    RRMode ideal_mode;
    int x;
    int y;
    bool is_connected;
    bool is_powered_on;
    bool is_internal;
    unsigned long mm_width;
    unsigned long mm_height;
  };
  // Updates |output_count_|, |output_cache_|, |mirror_supported_|,
  // |primary_output_index_|, and |secondary_output_index_| with new data.
  // Returns true if the update succeeded or false if it was skipped since no
  // actual change was observed.
  // Note that |output_state_| is not updated by this call.
  bool TryRecacheOutputs(const DisplayInfo& info);

  // Uses the data stored in |output_cache_| and the given |new_state| to
  // configure the output interface and then updates |output_state_| to reflect
  // the new state.
  virtual void UpdateCacheAndOutputToState(const DisplayInfo& info,
                                           State new_state);

  // A helper to re-cache instance variable state and transition into the
  // appropriate default state for the observed displays.
  virtual bool RecacheAndUseDefaultState();

  // Called during start-up to determine what the current state of the displays
  // appears to be, by investigating how the outputs compare to the data stored
  // in |output_cache_|.  Returns STATE_INVALID if the current display state
  // doesn't match any supported state.  |output_cache_| must be up-to-date with
  // regards to the state of X or this method may return incorrect results.
  virtual State InferCurrentState(const DisplayInfo& info) const;

  virtual bool IsProjecting() const;
  // The number of outputs in the output_cache_ array.
  int output_count_;

  // The list of cached output descriptions (|output_count_| elements long).
  scoped_array<CachedOutputDescription> output_cache_;

  // The base of the event numbers used to represent XRandr events used in
  // decoding events regarding output add/remove.
  int xrandr_event_base_;

  DISALLOW_COPY_AND_ASSIGN(OutputConfiguratorX11);
};

}  // namespace chromeos

#endif  // CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_X11_H_
