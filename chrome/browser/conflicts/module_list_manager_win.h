// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MODULE_LIST_MANAGER_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MODULE_LIST_MANAGER_WIN_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "base/version.h"

namespace component_updater {
class ThirdPartyModuleListComponentInstallerTraits;
}  // namespace component_updater

// Declares a class that is responsible for knowing the location of the most
// up-to-date module list (a whitelist of third party modules that are safe for
// injection). This is a singleton that is created in very early startup
// (PreMessageLoopRunImpl, as part of RegisterComponentsForUpdate). It is not
// owned by the ModuleDatabase, as it is actually created long before the
// database.
//
// Since the module list can change at runtime (a new component version can be
// installed) this class provides an observer interface that can notify clients
// of a new whitelist. It is expected that ModuleDatabase and ModuleList be
// observers.
//
// This class provides a point of abstraction that allows entirely different
// module lists to be provided for testing and for enterprise installs.
//
// This class provides thread-safety because the component updater service
// (which does work on a private background priority sequence) and the
// ModuleDatabase initialization (which runs in the IO thread after the task
// scheduler has been initialized) race to create and access this class.
class ModuleListManager {
 public:
  // The root of the module list registry information. This is relative to the
  // root of the installation registry data, as returned by
  // install_static::InstallDetails::GetClientStateKeyPath.
  static const wchar_t kModuleListRegistryKeyPath[];

  // The name of the key below registry_key_root_ that stores the path to the
  // most recent module list, as a string.
  static const wchar_t kModuleListPathKeyName[];

  // The name of the key below registry_key_root_ that stores the version of the
  // most recent module list, as a string.
  static const wchar_t kModuleListVersionKeyName[];

  // An observer of changes to the ModuleListManager. The observer will only
  // be called if the module list installation location changes after the
  // observer has been registered.
  class Observer {
   public:
    Observer();
    virtual ~Observer();

    // This is invoked as a background priority task (not on IO or UI threads)
    // where file IO and blocking is allowed. The sequence is owned by the
    // component updater service.
    virtual void OnNewModuleList(const base::Version& version,
                                 const base::FilePath& path) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  ~ModuleListManager();

  // Factory and singleton accessors. This is thread safe.
  static ModuleListManager* GetInstance();

  // Returns the path to the current module list. This is empty if no module
  // list is available. This is thread safe.
  base::FilePath module_list_path();

  // Returns the version of the current module list. This is empty if no module
  // list is available. This is thread safe.
  base::Version module_list_version();

  // For adding and removing an observer. It is expected that there be only a
  // single observer. The observer must outlive this class. Call
  // SetObserver(nullptr) to remove the existing observer, if any. This is
  // thread safe.
  void SetObserver(Observer* observer);

  // Generates the registry key where Path and Version information will be
  // written. Exposed for testing.
  static HKEY GetRegistryHive();
  static std::wstring GetRegistryPath();

 protected:
  friend class component_updater::ThirdPartyModuleListComponentInstallerTraits;

  // Called post-startup with information about the most recently available
  // module list installation. Can potentially be called again must later when
  // another version is installed.
  void LoadModuleList(const base::Version& version, const base::FilePath& path);

 private:
  friend class ModuleListManagerTest;

  // Private to enforce singleton semantics.
  ModuleListManager();

  // Tears down the singleton ModuleListManager for testing. Thread safe.
  static void TearDownForTesting();

  // The hive and path to the registry key where the most recent module list
  // path information is cached.
  const std::wstring registry_key_root_;

  // Provides synchronization for this class. The updates to the module list can
  // occur at any moment from a private background priority sequence owned by
  // the component updater service. Meanwhile, the ModuleDatabase that will be
  // using the manager does work the UI threads and other background sequences.
  base::Lock lock_;

  // The path and version of the most recently installed module list. This is
  // retrieved from the registry at creation of ModuleListManager, and
  // potentially updated at runtime via calls to LoadModuleList.
  base::FilePath module_list_path_;    // Under lock_.
  base::Version module_list_version_;  // Under lock_.

  // The observer of this object. Not called under lock_.
  Observer* observer_;  // Under lock_.

  DISALLOW_COPY_AND_ASSIGN(ModuleListManager);
};

#endif  // CHROME_BROWSER_CONFLICTS_MODULE_LIST_MANAGER_WIN_H_
