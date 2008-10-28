// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#ifndef CHROME_BROWSER_BACKGROUND_TASK_BACKGROUND_TASK_MANAGER_
#define CHROME_BROWSER_BACKGROUND_TASK_BACKGROUND_TASK_MANAGER_

#include <map>
#include <vector>
#include "base/basictypes.h"
#include "chrome/common/notification_types.h"
#include "googleurl/src/gurl.h"

class BackgroundTaskRunner;
class PrefService;
class Profile;
class RenderViewHost;
class WebContents;

// Defines the starting behavior of the background task.
enum BackgroundTaskStartMode {
  START_BACKGROUND_TASK_ON_DEMAND,
  START_BACKGROUND_TASK_ON_BROWSER_LAUNCH,
  START_BACKGROUND_TASK_ON_LOGIN,
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
                 const GURL& url,
                 BackgroundTaskStartMode start_mode)
      : id(id),
        url(url),
        start_mode(start_mode),
        runner(NULL) {
  }

  RenderViewHost* render_view_host() const;
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
                    const GURL& url,
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

#endif  // CHROME_BROWSER_BACKGROUND_TASK_BACKGROUND_TASK_MANAGER_

#endif  // ENABLE_BACKGROUND_TASK
