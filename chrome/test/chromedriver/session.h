// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_SESSION_H_
#define CHROME_TEST_CHROMEDRIVER_SESSION_H_

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "chrome/test/chromedriver/basic_types.h"
#include "chrome/test/chromedriver/chrome/device_metrics.h"
#include "chrome/test/chromedriver/chrome/geoposition.h"
#include "chrome/test/chromedriver/chrome/network_conditions.h"
#include "chrome/test/chromedriver/chrome/scoped_temp_dir_with_retry.h"
#include "chrome/test/chromedriver/command_listener.h"

static const char kAccept[] = "accept";
static const char kDismiss[] = "dismiss";
static const char kIgnore[] = "ignore";

namespace base {
class DictionaryValue;
}

class Chrome;
class Status;
class WebDriverLog;
class WebView;

struct FrameInfo {
  FrameInfo(const std::string& parent_frame_id,
            const std::string& frame_id,
            const std::string& chromedriver_frame_id);

  std::string parent_frame_id;
  std::string frame_id;
  std::string chromedriver_frame_id;
};

// TODO(kereliuk): make this a base::Value subclass
struct ActionObject {
  ActionObject(const std::string& id,
               const std::string& type,
               const std::string& subtype);

  const std::string id;
  const std::string type;
  const std::string subtype;
};

struct InputSource {
  InputSource();
  InputSource(const std::string& input_id, const std::string& source_type);
  // InputSource(const std::string& input_id, const std::string& source_type,
  // const std::string& pointer_type);

  std::string input_id;
  // one of 'key', 'pointer' or 'none'
  std::string source_type;
  // one of 'mouse', 'pen' or 'touch'
  std::string pointer_type;
};

struct PointerInputSource : InputSource {
  PointerInputSource(const std::string& input_id,
                     const std::string& source_type,
                     const std::string& pointer_type);

  std::string input_id;
  // one of 'key', 'pointer' or 'none'
  std::string source_type;
  std::string pointer_type;
};

struct InputState {};

struct KeyInputState : InputState {
  KeyInputState();
  KeyInputState(std::set<std::string> pressed,
                bool alt,
                bool shift,
                bool ctrl,
                bool meta);
  ~KeyInputState();

  // strings representing the currently pressed keys
  std::set<std::string> pressed;
  bool alt;
  bool shift;
  bool ctrl;
  bool meta;
};

struct PointerInputState : InputState {
  PointerInputState();
  PointerInputState(std::string subtype,
                    std::set<unsigned int> pressed,
                    unsigned int x,
                    unsigned int y);
  ~PointerInputState();

  // one of "mouse", "pen" or "touch"
  std::string subtype;
  std::set<unsigned int> pressed;
  unsigned int x;
  unsigned int y;
};

struct Session {
  static const base::TimeDelta kDefaultPageLoadTimeout;

  explicit Session(const std::string& id);
  Session(const std::string& id, std::unique_ptr<Chrome> chrome);
  ~Session();

  Status GetTargetWindow(WebView** web_view);

  void SwitchToTopFrame();
  void SwitchToParentFrame();
  void SwitchToSubFrame(const std::string& frame_id,
                        const std::string& chromedriver_frame_id);
  std::string GetCurrentFrameId() const;
  std::vector<WebDriverLog*> GetAllLogs() const;
  std::string GetFirstBrowserError() const;

  const std::string id;
  bool w3c_compliant;
  bool quit;
  bool detach;
  bool force_devtools_screenshot;
  std::unique_ptr<Chrome> chrome;
  std::string window;
  int sticky_modifiers;
  // List of input sources for each active input. Everytime a new input source
  // is added, there must be a corresponding entry made in input_state_table.
  std::unique_ptr<base::ListValue> active_input_sources;
  // Map between input id and input source state for the corresponding input
  // source. One entry for each item in active_input_sources
  std::unique_ptr<base::DictionaryValue> input_state_table;
  // List of |FrameInfo|s for each frame to the current target frame from the
  // first frame element in the root document. If target frame is window.top,
  // this list will be empty.
  std::list<FrameInfo> frames;
  WebPoint mouse_position;
  base::TimeDelta implicit_wait;
  base::TimeDelta page_load_timeout;
  base::TimeDelta script_timeout;
  std::unique_ptr<std::string> prompt_text;
  std::unique_ptr<Geoposition> overridden_geoposition;
  std::unique_ptr<DeviceMetrics> overridden_device_metrics;
  std::unique_ptr<NetworkConditions> overridden_network_conditions;
  std::string orientation_type;
  // Logs that populate from DevTools events.
  std::vector<std::unique_ptr<WebDriverLog>> devtools_logs;
  std::unique_ptr<WebDriverLog> driver_log;
  ScopedTempDirWithRetry temp_dir;
  std::unique_ptr<base::DictionaryValue> capabilities;
  bool auto_reporting_enabled;
  // |command_listeners| should be declared after |chrome|. When the |Session|
  // is destroyed, |command_listeners| should be freed first, since some
  // |CommandListener|s might be |CommandListenerProxy|s that forward to
  // |DevToolsEventListener|s owned by |chrome|.
  std::vector<std::unique_ptr<CommandListener>> command_listeners;
  std::string unexpected_alert_behaviour;
};

Session* GetThreadLocalSession();

void SetThreadLocalSession(std::unique_ptr<Session> session);

#endif  // CHROME_TEST_CHROMEDRIVER_SESSION_H_
