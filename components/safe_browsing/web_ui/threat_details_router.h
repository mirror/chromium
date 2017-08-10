// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_WEBUI_THREAT_DETAILS_ROUTER_H_
#define COMPONENTS_SAFE_BROWSING_WEBUI_THREAT_DETAILS_ROUTER_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
//#include "components/keyed_service/core/keyed_service.h"

namespace safe_browsing {
class LogManager;
class LogReceiver;

// The router stands between LogManager and LogReceiver instances. Both managers
// and receivers need to register (and unregister) with the router. After that,
// the following communication is enabled:
//   * LogManagers are notified when logging starts or stops being possible
//   * LogReceivers are sent logs routed through LogRouter
class LogRouter {
 public:
  LogRouter();
  ~LogRouter();

  // Passes logs to the router. Only call when there are receivers registered.
  void ProcessLog(const std::string& text);

  // All four (Unr|R)egister* methods below are safe to call from the
  // constructor of the registered object, because they do not call that object,
  // and the router only runs on a single thread.

  // The managers must register to be notified about whether there are some
  // receivers or not. RegisterManager adds |manager| to the right observer list
  // and returns true iff there are some receivers registered.
  bool RegisterManager(LogManager* manager);
  // Remove |manager| from the observers list.
  void UnregisterManager(LogManager* manager);

  // The receivers must register to get updates with new logs in the future.
  // RegisterReceiver adds |receiver| to the right observer list, and returns
  // the logs accumulated so far. (It returns by value, not const ref, to
  // provide a snapshot as opposed to a link to |accumulated_logs_|.)
  std::string RegisterReceiver(LogReceiver* receiver) WARN_UNUSED_RESULT;
  // Remove |receiver| from the observers list.
  void UnregisterReceiver(LogReceiver* receiver);

 private:
  // Observer lists for managers and receivers. The |true| in the template
  // specialisation means that they will check that all observers were removed
  // on destruction.
  base::ObserverList<LogManager, true> managers_;
  base::ObserverList<LogReceiver, true> receivers_;

  // Logs accumulated since the first receiver was registered.
  std::string accumulated_logs_;

  DISALLOW_COPY_AND_ASSIGN(LogRouter);
};

// Collects the logs for the password manager internals page and distributes
// them to all open tabs with the internals page.
class PasswordManagerInternalsService : public KeyedService, public LogRouter {
 public:
  // There are only two ways in which the service depends on the BrowserContext:
  // 1) There is one service per each non-incognito BrowserContext.
  // 2) No service will be created for an incognito BrowserContext.
  // Both properties are guarantied by the BrowserContextKeyedFactory framework,
  // so the service itself does not need the context on creation.
  PasswordManagerInternalsService();
  ~PasswordManagerInternalsService() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordManagerInternalsService);
};

// This interface is used by the password management code to receive and display
// logs about progress of actions like saving a password.
class LogReceiver {
 public:
  LogReceiver() {}
  virtual ~LogReceiver() {}

  virtual void LogSavePasswordProgress(const std::string& text) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(LogReceiver);
};

// This interface is used by the password management code to receive and display
// logs about progress of actions like saving a password.
class LogManager {
 public:
  virtual ~LogManager() = default;

  // This method is called by a LogRouter, after the LogManager registers with
  // one. If |router_can_be_used| is true, logs can be sent to LogRouter after
  // the return from OnLogRouterAvailabilityChanged and will reach at least one
  // LogReceiver instance. If |router_can_be_used| is false, no logs should be
  // sent to the LogRouter.
  virtual void OnLogRouterAvailabilityChanged(bool router_can_be_used) = 0;

  // The owner of the LogManager can call this to start or end suspending the
  // logging, by setting |suspended| to true or false, respectively.
  virtual void SetSuspended(bool suspended) = 0;

  // Forward |text| for display to the LogRouter (if registered with one).
  virtual void LogSavePasswordProgress(const std::string& text) const = 0;

  // Returns true if logs recorded via LogSavePasswordProgress will be
  // displayed, and false otherwise.
  virtual bool IsLoggingActive() const = 0;

  // Returns the production code implementation of LogManager. If |log_router|
  // is null, the manager will do nothing. |notification_callback| will be
  // called every time the activity status of logging changes.
  static std::unique_ptr<LogManager> Create(
      LogRouter* log_router,
      base::Closure notification_callback);
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_WEBUI_THREAT_DETAILS_ROUTER_H_
