// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if ENABLE_BACKGROUND_TASK == 1

#ifndef CHROME_BROWSER_BACKGROUND_TASK_MANAGER_H_
#define CHROME_BROWSER_BACKGROUND_TASK_MANAGER_H_

#include <map>
#include "base/basictypes.h"

class Browser;
class GURL;
class ListValue;
class PrefService;
class Profile;
class WebContents;

struct BackgroundTask {
  std::wstring id;
  std::wstring url;
  bool started;
};

// Background task manager.
class BackgroundTaskManager {
 public:
  BackgroundTaskManager(Browser* browser, Profile* profile);
  ~BackgroundTaskManager();

  static void RegisterUserPrefs(PrefService* prefs);

  // Registers a background task.
  bool RegisterTask(WebContents* source,
                    const std::wstring& task_id,
                    const std::wstring& task_url);

  // Unregisters a background task.
  bool UnregisterTask(WebContents* source, const std::wstring& task_id);

  // Starts a background task.
  bool StartTask(WebContents* source, const std::wstring& task_id);

  // Ends a background task.
  bool EndTask(WebContents* source, const std::wstring& task_id);

 private:
  // Loads the tasks from the user preferences store.
  void LoadUserPrefs();

  // Helper function to get the ID used in the map.
  static std::wstring ConstructMapID(const GURL& url,
                                     const std::wstring& id);

  // Stores all the background tasks being registered.
  typedef std::map<std::wstring, BackgroundTask> BackgroundTaskMap;
  BackgroundTaskMap tasks_;

  // Owning browser.
  Browser* browser_;

  // Used to access preferences store.
  PrefService* prefs_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTaskManager);
};

#endif  // CHROME_BROWSER_BACKGROUND_TASK_MANAGER_H_

#endif  // ENABLE_BACKGROUND_TASK == 1
