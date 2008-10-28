// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#ifndef CHROME_BROWSER_BACKGROUND_TASK_MANAGER_H_
#define CHROME_BROWSER_BACKGROUND_TASK_MANAGER_H_

#include <map>
#include <vector>
#include "base/basictypes.h"
#include "base/task.h"
#include "chrome/browser/render_view_host_delegate.h"
#include "chrome/common/notification_types.h"
#include "googleurl/src/gurl.h"
#include "webkit/glue/webpreferences.h"

struct BackgroundTask;
class BackgroundTaskManager;
class ListValue;
class NotificationDetails;
class NotificationSource;
class PrefService;
class Profile;
class RenderViewHost;

// Defines the starting behavior of the background task.
enum BackgroundTaskStartMode {
  START_BACKGROUND_TASK_ON_DEMAND,
  START_BACKGROUND_TASK_ON_BROWSER_LAUNCH,
  START_BACKGROUND_TASK_ON_LOGIN,
};

// Defines a light-weight wrapper around a RenderViewHost responsible for
// running HTML and JS of a background task.
class BackgroundTaskRunner : public RenderViewHostDelegate {
 public:
  BackgroundTaskRunner(BackgroundTaskManager* background_task_manager,
                       BackgroundTask* background_task);
  virtual ~BackgroundTaskRunner() {}

  // Helper to find the background task that originated the given request.
  static BackgroundTask* GetBackgroundTaskByID(int render_process_id,
                                               int render_view_id);

  // Starts the background task.
  bool Start();

  // Shuts down the background task.
  void Shutdown();

  // RenderViewHostDelegate implementations.
  virtual WebPreferences GetWebkitPrefs();
  virtual Profile* GetProfile() const;
  virtual void RendererReady(RenderViewHost* render_view_host);
  virtual void RendererGone(RenderViewHost* render_view_host);
  virtual void UpdateTitle(RenderViewHost* render_view_host,
                           int32 page_id,
                           const std::wstring& title);

  // Accessors.
  RenderViewHost* render_view_host() const { return render_view_host_; }
  const std::wstring& title() const { return title_; }

 private:
  // Helper functions for sending notifications.
  void NotifyConnected();
  void NotifyDisconnected();

  // Does the delayed shutdown.
  void DelayedShutdown();

  // The background task manager.
  BackgroundTaskManager* background_task_manager_;

  // The background task.
  BackgroundTask* background_task_;

  // The host HTML runner.
  RenderViewHost* render_view_host_;

  // Indicates whether we should notify about disconnection of this background
  // task. This is used to ensure disconnection notifications only happen if
  // a connection notification has happened and that they happen only once.
  bool notify_disconnection_;

  // The title.
  std::wstring title_;

  // The following factory is used to call methods at a later time.
  ScopedRunnableMethodFactory<BackgroundTaskRunner> method_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTaskRunner);
};

// Defines the characteristics of a background task.
struct BackgroundTask {
  std::wstring id;
  GURL url;
  BackgroundTaskStartMode start_mode;
  BackgroundTaskRunner* runner;

  BackgroundTask()
      : start_mode(START_BACKGROUND_TASK_ON_BROWSER_LAUNCH),
        runner(NULL) {
  }

  BackgroundTask(const std::wstring& id,
                 const std::wstring& url,
                 BackgroundTaskStartMode start_mode)
      : id(id),
        url(url),
        start_mode(start_mode),
        runner(NULL) {
  }

  RenderViewHost* render_view_host() const {
    return runner ? runner->render_view_host() : NULL;
  }
};

// Background task manager.
class BackgroundTaskManager {
 public:
  BackgroundTaskManager(Profile* profile);
  ~BackgroundTaskManager();

  // Registers the prefs for the background tasks.
  static void RegisterUserPrefs(PrefService* prefs);

  // Returns all background tasks.
  static void GetAllBackgroundTasks(std::vector<BackgroundTask*>* tasks);

  // Closes all running background tasks.
  static void CloseAllActiveTasks();

  // Loads and runs the previously registered background tasks.
  // This is done at the browser boot up time.
  void LoadAndStartAllRegisteredTasks();

  // Registers a background task.
  bool RegisterTask(WebContents* source,
                    const std::wstring& id,
                    const std::wstring& url,
                    BackgroundTaskStartMode start_type);

  // Unregisters a background task.
  bool UnregisterTask(WebContents* source, const std::wstring& id);

  // Starts a background task.
  bool StartTask(WebContents* source, const std::wstring& id);

  // Ends a background task.
  bool EndTask(WebContents* source, const std::wstring& id);

  // Finds the background task given its ID and URL.
  BackgroundTask* FindTask(const std::wstring& id,
                           const GURL& url);

  // Returns true if the specified background task is running.
  bool IsTaskRunning(WebContents* source, const std::wstring& id) const;

  // Accessors.
  Profile* profile() const { return profile_; }

 private:
  // Helper function to get the ID used in the map.
  static std::wstring ConstructMapID(const GURL& url,
                                     const std::wstring& id);

  // Stores all the background tasks being registered.
  typedef std::map<std::wstring, BackgroundTask> BackgroundTaskMap;
  BackgroundTaskMap tasks_;

  // Associated profile.
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTaskManager);
};

#endif  // CHROME_BROWSER_BACKGROUND_TASK_MANAGER_H_

#endif  // ENABLE_BACKGROUND_TASK
