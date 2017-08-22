// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MODULE_LIST_MANAGER_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MODULE_LIST_MANAGER_WIN_H_

#include "base/files/file_path.h"
#include "base/observer_list.h"
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
  // observer has been registered. The observer must manually query the manager
  // to retrieve the initial installation location.
  class Observer {
   public:
    Observer();
    virtual ~Observer();
    // DO NOT SUBMIT: Document threads!
    virtual void OnNewModuleList(const base::Version& version,
                                 const base::FilePath& path) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  ~ModuleListManager();

  // Factory and singleton accessors.
  static ModuleListManager* GetInstance();

  // Returns the path to the current module list. This is empty if no module
  // list is available.
  const base::FilePath& module_list_path() const { return module_list_path_; }

  // Returns the version of the current module list. This is empty if no module
  // list is available.
  const base::Version& module_list_version() const {
    return module_list_version_;
  }

  // For adding and removing observers. The observer must outlive this class or
  // be removed prior to destroying the observer.
  // DO NOT SUBMIT: Document the thread on which these may be called!
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

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

  // Tears down the singleton ModuleListManager for testing.
  static void TearDownForTesting();

  // The hive and path to the registry key where the most recent module list
  // path information is cached.
  std::wstring registry_key_root_;

  // The path and version of the most recently installed module list. This is
  // retrieved from the registry at creation of ModuleListManager, and
  // potentially updated at runtime via calls to LoadModuleList.
  base::FilePath module_list_path_;
  base::Version module_list_version_;

  // The list of registered observers.
  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(ModuleListManager);
};

#endif  // CHROME_BROWSER_CONFLICTS_MODULE_LIST_MANAGER_WIN_H_
