// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_H_
#define CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_H_

#include "base/basictypes.h"
#include "base/event_types.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

// Used to describe the state of a multi-display configuration.
enum State {
  STATE_INVALID,
  STATE_HEADLESS,
  STATE_SINGLE,
  STATE_DUAL_MIRROR,
  STATE_DUAL_PRIMARY_ONLY,
  STATE_DUAL_SECONDARY_ONLY,
};

// A generic interface for reading and modifying display settings.
class CHROMEOS_EXPORT OutputConfigurator : public MessageLoop::Dispatcher {
 public:
  OutputConfigurator();
  virtual ~OutputConfigurator();

  State output_state() const { return output_state_; }

  // Called when the user hits ctrl-F4 to request a display mode change.
  // This method should only return false if it was called in a single-head or
  // headless mode.
  virtual bool CycleDisplayMode();

  // Called when powerd notifies us that some set of displays should be turned
  // on or off.  This requires enabling or disabling the CRTC associated with
  // the display(s) in question so that the low power state is engaged.
  virtual bool ScreenPowerSet(bool power_on, bool all_displays) = 0;

  // Force switching the display mode to |new_state|.  This method is used when
  // the user explicitly changes the display mode in the options UI.  Returns
  // false if it was called in a single-head or headless mode.
  virtual bool SetDisplayMode(State new_state) = 0;

  // Called when an RRNotify event is received.  The implementation is
  // interested in the cases of RRNotify events which correspond to output
  // add/remove events.  Note that Output add/remove events are sent in response
  // to our own reconfiguration operations so spurious events are common.
  // Spurious events will have no effect.
  virtual bool Dispatch(const base::NativeEvent& event) = 0;

 protected:
  // Container for implementation-specific display system info.
  struct DisplayInfo;

  // Container for caching implementation-specific output info.
  struct CachedOutputDescription {
    bool is_connected;
    bool is_powered_on;
    bool is_internal;
  };

  // Updates |output_count_|, |output_cache_|, |mirror_supported_|,
  // |primary_output_index_|, and |secondary_output_index_| with new data.
  // Returns true if the update succeeded or false if it was skipped since no
  // actual change was observed.
  // Note that |output_state_| is not updated by this call.
  virtual bool TryRecacheOutputs(const DisplayInfo& info) { return true; }

  // Uses the data stored in |output_cache_| and the given |new_state| to
  // configure the output interface and then updates |output_state_| to reflect
  // the new state.
  virtual void UpdateCacheAndOutputToState(const DisplayInfo& info,
                                           State new_state) {}

  // A helper to re-cache instance variable state and transition into the
  // appropriate default state for the observed displays.
  virtual bool RecacheAndUseDefaultState() { return true; }

  // Checks the |primary_output_index_|, |secondary_output_index_|, and
  // |mirror_supported_| to see how many displays are currently connected and
  // returns the state which is most appropriate as a default state for those
  // displays.
  virtual State GetDefaultState() const;

  // Called during start-up to determine what the current state of the displays
  // appears to be, by investigating how the outputs compare to the data stored
  // in |output_cache_|.  Returns STATE_INVALID if the current display state
  // doesn't match any supported state.  |output_cache_| must be up-to-date with
  // regards to actual output state or this method may return incorrect results.
  virtual State InferCurrentState(const DisplayInfo& info) const { return STATE_INVALID; }

  // Calls IsProjecting() to determine whether or not we are in a "projecting"
  // state and then calls the DBus kSetIsProjectingMethod on powerd with the
  // result.
  virtual void CheckIsProjectingAndNotify();

  // Determines whether the system is projecting to an external display.
  virtual bool IsProjecting() const = 0;

  // The display state as derived from the outputs observed in |output_cache_|.
  // This is used for rotating display modes.
  State output_state_;

  // True if |output_cache_| describes a permutation of outputs which support a
  // mirrored device mode.
  bool mirror_supported_;

  // The index of the primary connected output in |output_cache_|.  -1 if there
  // is no primary output.  This implies the machine currently has no outputs.
  int primary_output_index_;

  // The index of the secondary connected output in |output_cache_|.  -1 if
  // there is no secondary output.  This implies the machine currently has one
  // output.
  int secondary_output_index_;

  DISALLOW_COPY_AND_ASSIGN(OutputConfigurator);
};

}  // namespace chromeos

#endif  // CHROMEOS_DISPLAY_OUTPUT_CONFIGURATOR_H_
