// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/session.h"

#include <list>
#include <utility>

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"
#include "base/values.h"
#include "chrome/test/chromedriver/chrome/chrome.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/chrome/web_view.h"
#include "chrome/test/chromedriver/logging.h"

namespace {

base::LazyInstance<base::ThreadLocalPointer<Session>>::DestructorAtExit
    lazy_tls_session = LAZY_INSTANCE_INITIALIZER;

}  // namespace

FrameInfo::FrameInfo(const std::string& parent_frame_id,
                     const std::string& frame_id,
                     const std::string& chromedriver_frame_id)
    : parent_frame_id(parent_frame_id),
      frame_id(frame_id),
      chromedriver_frame_id(chromedriver_frame_id) {}

ActionObject::ActionObject(const std::string& id,
                           const std::string& type,
                           const std::string& subtype)
    : id(id), type(type), subtype(subtype) {}

InputSource::InputSource(const std::string& input_id,
                         const std::string& source_type)
    : input_id(input_id), source_type(source_type), pointer_type("mouse") {}

InputSource::InputSource() {}

KeyInputState::KeyInputState(std::set<std::string> pressed,
                             bool alt,
                             bool shift,
                             bool ctrl,
                             bool meta)
    : pressed(pressed), alt(alt), shift(shift), ctrl(ctrl), meta(meta) {}

KeyInputState::KeyInputState()
    : pressed(std::set<std::string>()),
      alt(false),
      shift(false),
      ctrl(false),
      meta(false) {}

KeyInputState::~KeyInputState() {}

PointerInputState::PointerInputState(std::string subtype,
                                     std::set<unsigned int> pressed,
                                     unsigned int x,
                                     unsigned int y)
    : subtype(subtype), pressed(pressed), x(x), y(y) {}

PointerInputState::PointerInputState()
    : subtype("subtype"), pressed(std::set<unsigned int>()), x(0), y(0) {}

PointerInputState::~PointerInputState() {}

const base::TimeDelta Session::kDefaultPageLoadTimeout =
    base::TimeDelta::FromMinutes(5);

Session::Session(const std::string& id)
    : id(id),
      w3c_compliant(false),
      quit(false),
      detach(false),
      force_devtools_screenshot(false),
      sticky_modifiers(0),
      active_input_sources(new base::ListValue()),
      input_state_table(new base::DictionaryValue()),
      mouse_position(0, 0),
      page_load_timeout(kDefaultPageLoadTimeout),
      auto_reporting_enabled(false) {}

Session::Session(const std::string& id, std::unique_ptr<Chrome> chrome)
    : id(id),
      w3c_compliant(false),
      quit(false),
      detach(false),
      force_devtools_screenshot(false),
      chrome(std::move(chrome)),
      sticky_modifiers(0),
      active_input_sources(new base::ListValue()),
      input_state_table(new base::DictionaryValue()),
      mouse_position(0, 0),
      page_load_timeout(kDefaultPageLoadTimeout),
      auto_reporting_enabled(false) {}

Session::~Session() {}

Status Session::GetTargetWindow(WebView** web_view) {
  if (!chrome)
    return Status(kNoSuchWindow, "no chrome started in this session");

  Status status = chrome->GetWebViewById(window, web_view);
  if (status.IsError())
    status = Status(kNoSuchWindow, "target window already closed", status);
  return status;
}

void Session::SwitchToTopFrame() {
  frames.clear();
}

void Session::SwitchToParentFrame() {
  if (!frames.empty())
    frames.pop_back();
}

void Session::SwitchToSubFrame(const std::string& frame_id,
                               const std::string& chromedriver_frame_id) {
  std::string parent_frame_id;
  if (!frames.empty())
    parent_frame_id = frames.back().frame_id;
  frames.push_back(FrameInfo(parent_frame_id, frame_id, chromedriver_frame_id));
}

std::string Session::GetCurrentFrameId() const {
  if (frames.empty())
    return std::string();
  return frames.back().frame_id;
}

std::vector<WebDriverLog*> Session::GetAllLogs() const {
  std::vector<WebDriverLog*> logs;
  for (const auto& log : devtools_logs)
    logs.push_back(log.get());
  if (driver_log)
    logs.push_back(driver_log.get());
  return logs;
}

std::string Session::GetFirstBrowserError() const {
  for (const auto& log : devtools_logs) {
    if (log->type() == WebDriverLog::kBrowserType) {
      std::string message = log->GetFirstErrorMessage();
      if (!message.empty())
        return message;
    }
  }
  return std::string();
}

Session* GetThreadLocalSession() {
  return lazy_tls_session.Pointer()->Get();
}

void SetThreadLocalSession(std::unique_ptr<Session> session) {
  lazy_tls_session.Pointer()->Set(session.release());
}
