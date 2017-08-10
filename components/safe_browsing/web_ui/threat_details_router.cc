// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/web_ui/threat_details_router.h"

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"

namespace safe_browsing {
PasswordManagerInternalsService::PasswordManagerInternalsService() {}

PasswordManagerInternalsService::~PasswordManagerInternalsService() {}

namespace {

class LogManagerImpl : public LogManager {
 public:
  LogManagerImpl(LogRouter* log_router, base::Closure notification_callback);

  ~LogManagerImpl() override;

  // LogManager
  void OnLogRouterAvailabilityChanged(bool router_can_be_used) override;
  void SetSuspended(bool suspended) override;
  void LogSavePasswordProgress(const std::string& text) const override;
  bool IsLoggingActive() const override;

 private:
  // A LogRouter instance obtained on construction. May be null.
  LogRouter* const log_router_;

  // True if |this| is registered with some LogRouter which can accept logs.
  bool can_use_log_router_;

  bool is_suspended_ = false;

  // Called every time the logging activity status changes.
  base::Closure notification_callback_;

  DISALLOW_COPY_AND_ASSIGN(LogManagerImpl);
};

LogManagerImpl::LogManagerImpl(LogRouter* log_router,
                               base::Closure notification_callback)
    : log_router_(log_router),
      can_use_log_router_(log_router_ && log_router_->RegisterManager(this)),
      notification_callback_(notification_callback) {}

LogManagerImpl::~LogManagerImpl() {
  if (log_router_)
    log_router_->UnregisterManager(this);
}

void LogManagerImpl::OnLogRouterAvailabilityChanged(bool router_can_be_used) {
  DCHECK(log_router_);  // |log_router_| should be calling this method.
  if (can_use_log_router_ == router_can_be_used)
    return;
  can_use_log_router_ = router_can_be_used;

  if (!is_suspended_) {
    // The availability of the logging changed as a result.
    if (!notification_callback_.is_null())
      notification_callback_.Run();
  }
}

void LogManagerImpl::SetSuspended(bool suspended) {
  if (suspended == is_suspended_)
    return;
  is_suspended_ = suspended;
  if (can_use_log_router_) {
    // The availability of the logging changed as a result.
    if (!notification_callback_.is_null())
      notification_callback_.Run();
  }
}

void LogManagerImpl::LogSavePasswordProgress(const std::string& text) const {
  if (!IsLoggingActive())
    return;
  log_router_->ProcessLog(text);
}

bool LogManagerImpl::IsLoggingActive() const {
  return can_use_log_router_ && !is_suspended_;
}

}  // namespace

// static
std::unique_ptr<LogManager> LogManager::Create(
    LogRouter* log_router,
    base::Closure notification_callback) {
  return base::WrapUnique(
      new LogManagerImpl(log_router, notification_callback));
}

LogRouter::LogRouter() = default;

LogRouter::~LogRouter() = default;

void LogRouter::ProcessLog(const std::string& text) {
  // This may not be called when there are no receivers (i.e., the router is
  // inactive), because in that case the logs cannot be displayed.
  DCHECK(receivers_.might_have_observers());
  accumulated_logs_.append(text);
  for (LogReceiver& receiver : receivers_)
    receiver.LogSavePasswordProgress(text);
}

bool LogRouter::RegisterManager(LogManager* manager) {
  DCHECK(manager);
  managers_.AddObserver(manager);
  return receivers_.might_have_observers();
}

void LogRouter::UnregisterManager(LogManager* manager) {
  DCHECK(managers_.HasObserver(manager));
  managers_.RemoveObserver(manager);
}

std::string LogRouter::RegisterReceiver(LogReceiver* receiver) {
  DCHECK(receiver);
  DCHECK(accumulated_logs_.empty() || receivers_.might_have_observers());

  if (!receivers_.might_have_observers()) {
    for (LogManager& manager : managers_)
      manager.OnLogRouterAvailabilityChanged(true);
  }
  receivers_.AddObserver(receiver);
  return accumulated_logs_;
}

void LogRouter::UnregisterReceiver(LogReceiver* receiver) {
  DCHECK(receivers_.HasObserver(receiver));
  receivers_.RemoveObserver(receiver);
  if (!receivers_.might_have_observers()) {
    // |accumulated_logs_| can become very long; use the swap instead of clear()
    // to ensure that the memory is freed.
    std::string().swap(accumulated_logs_);
    for (LogManager& manager : managers_)
      manager.OnLogRouterAvailabilityChanged(false);
  }
}

}  // namespace safe_browsing
